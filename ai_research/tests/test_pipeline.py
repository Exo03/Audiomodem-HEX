import os
import subprocess
import pytest
import numpy as np
import soundfile as sf
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'src'))
from src.augment import add_white_noise, add_jitter_delay, TARGET_SR

# Настройка путей
TEST_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TEST_DIR)
CPP_MODULATOR = os.path.join(ROOT_DIR, "cpp_core", "modulator")
CPP_DEMODULATOR = os.path.join(ROOT_DIR, "cpp_core", "demodulator")

TEST_BIN = os.path.join(TEST_DIR, "test_input.bin")
CLEAN_WAV = os.path.join(TEST_DIR, "clean_test.wav")
NOISY_WAV = os.path.join(TEST_DIR, "noisy_test.wav")
OUT_BIN = os.path.join(TEST_DIR, "test_output.bin")


@pytest.fixture(scope="session", autouse=True)
def compile_cpp():
    print("\nКомпиляция C++ исходников...")
    os.makedirs(os.path.join(ROOT_DIR, "cpp_core"), exist_ok=True)
    subprocess.run(["g++", "-O3", "cpp_core/modulator.cpp", "-o", CPP_MODULATOR], check=True)
    subprocess.run(["g++", "-O3", "cpp_core/demodulator.cpp", "-o", CPP_DEMODULATOR], check=True)
    yield
    for f in [TEST_BIN, CLEAN_WAV, NOISY_WAV, OUT_BIN]:
        if os.path.exists(f):
            os.remove(f)


def generate_random_file(filepath, size_bytes=1024):
    data = np.random.bytes(size_bytes)
    with open(filepath, 'wb') as f:
        f.write(data)
    return data


def calculate_success_rate(original_file, recovered_file):
    with open(original_file, 'rb') as f1, open(recovered_file, 'rb') as f2:
        orig = np.frombuffer(f1.read(), dtype=np.uint8)
        recv = np.frombuffer(f2.read(), dtype=np.uint8)

    min_len = min(len(orig), len(recv))
    max_len = max(len(orig), len(recv))

    matches = np.sum(orig[:min_len] == recv[:min_len])
    success_rate = (matches / max_len) * 100
    return success_rate


class TestAcousticModem:

    @pytest.mark.parametrize("snr_db", [30, 20, 10, 5])
    def test_end_to_end_noise(self, snr_db):
        generate_random_file(TEST_BIN, 100)

        subprocess.run([CPP_MODULATOR, TEST_BIN, CLEAN_WAV], check=True)

        audio, sr = sf.read(CLEAN_WAV)
        noisy_audio = add_white_noise(audio, snr_db)
        sf.write(NOISY_WAV, noisy_audio, sr)

        subprocess.run([CPP_DEMODULATOR, NOISY_WAV, OUT_BIN], check=True)

        success_rate = calculate_success_rate(TEST_BIN, OUT_BIN)

        assert success_rate >= 80.0, f"Провал при SNR {snr_db}dB. Успех: {success_rate}%"

    @pytest.mark.parametrize("jitter_ms", [0, 2, 5])
    def test_sync_jitter(self, jitter_ms):
        generate_random_file(TEST_BIN, 50)
        subprocess.run([CPP_MODULATOR, TEST_BIN, CLEAN_WAV], check=True)

        audio, sr = sf.read(CLEAN_WAV)
        jittered_audio = add_jitter_delay(audio, sr, max_delay_ms=jitter_ms)
        sf.write(NOISY_WAV, jittered_audio, sr)

        subprocess.run([CPP_DEMODULATOR, NOISY_WAV, OUT_BIN], check=True)

        success_rate = calculate_success_rate(TEST_BIN, OUT_BIN)
        assert success_rate >= 90.0, f"Фазовая рассинхронизация при {jitter_ms}ms"