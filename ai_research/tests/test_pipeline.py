import os
import subprocess
import pytest
import numpy as np
import soundfile as sf
from src.augment import add_white_noise, add_jitter_delay, TARGET_SR

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
AI_RESEARCH_DIR = os.path.dirname(TEST_DIR)
ROOT_DIR = os.path.dirname(AI_RESEARCH_DIR)

CPP_SRC_DIR = os.path.join(ROOT_DIR, "app", "src", "main", "cpp")

CPP_MODULATOR = os.path.join(TEST_DIR, "modulator.exe") if os.name == 'nt' else os.path.join(TEST_DIR, "modulator_bin")
CPP_DEMODULATOR = os.path.join(TEST_DIR, "demodulator.exe") if os.name == 'nt' else os.path.join(TEST_DIR,
                                                                                                 "demodulator_bin")

TEST_BIN = os.path.join(TEST_DIR, "test_input.bin")
CLEAN_WAV = os.path.join(TEST_DIR, "clean_test.wav")
NOISY_WAV = os.path.join(TEST_DIR, "noisy_test.wav")
OUT_BIN = os.path.join(TEST_DIR, "test_output.bin")


@pytest.fixture(scope="session", autouse=True)
def compile_cpp():
    modulator_src = os.path.join(CPP_SRC_DIR, "modulator.cpp")
    demodulator_src = os.path.join(CPP_SRC_DIR, "demodulator.cpp")

    assert os.path.exists(modulator_src)
    assert os.path.exists(demodulator_src)

    subprocess.run(["g++", "-O3", modulator_src, "-o", CPP_MODULATOR], check=True)
    subprocess.run(["g++", "-O3", demodulator_src, "-o", CPP_DEMODULATOR], check=True)

    yield

    for f in [TEST_BIN, CLEAN_WAV, NOISY_WAV, OUT_BIN, CPP_MODULATOR, CPP_DEMODULATOR]:
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

        assert success_rate >= 80.0

    @pytest.mark.parametrize("jitter_ms", [0, 2, 5])
    def test_sync_jitter(self, jitter_ms):
        generate_random_file(TEST_BIN, 50)
        subprocess.run([CPP_MODULATOR, TEST_BIN, CLEAN_WAV], check=True)

        audio, sr = sf.read(CLEAN_WAV)
        jittered_audio = add_jitter_delay(audio, sr, max_delay_ms=jitter_ms)
        sf.write(NOISY_WAV, jittered_audio, sr)

        subprocess.run([CPP_DEMODULATOR, NOISY_WAV, OUT_BIN], check=True)

        success_rate = calculate_success_rate(TEST_BIN, OUT_BIN)
        assert success_rate >= 90.0