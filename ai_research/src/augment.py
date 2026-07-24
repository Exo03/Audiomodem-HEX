import os
import glob
import numpy as np
import librosa
import soundfile as sf
from scipy import signal


def add_white_noise(audio_signal, snr_db):
    signal_power = np.mean(audio_signal ** 2)
    signal_power_db = 10 * np.log10(signal_power + 1e-10)
    noise_power_db = signal_power_db - snr_db
    noise_power = 10 ** (noise_power_db / 10)
    noise = np.random.normal(0, np.sqrt(noise_power), len(audio_signal))
    return audio_signal + noise


def add_reverberation(audio_signal, rir_signal):
    reverbed = signal.convolve(audio_signal, rir_signal, mode='full')
    reverbed = reverbed[:len(audio_signal)]
    max_val = np.max(np.abs(reverbed))
    return reverbed / max_val if max_val > 0 else reverbed


def add_jitter_delay(audio_signal, sr, max_delay_ms=20):
    delay_samples = int((np.random.uniform(0, max_delay_ms) / 1000.0) * sr)
    silence = np.zeros(delay_samples)
    jittered = np.concatenate((silence, audio_signal))
    return jittered[:len(audio_signal)]


def process_pipeline(clean_dir, rir_dir, output_dir, target_snr=15):
    os.makedirs(output_dir, exist_ok=True)
    clean_files = glob.glob(os.path.join(clean_dir, "*.wav"))
    rir_files = glob.glob(os.path.join(rir_dir, "*.wav"))

    if not clean_files:
        print(f"ОШИБКА: Нет чистых файлов в {clean_dir}")
        return

    for clean_path in clean_files:
        audio, sr = librosa.load(clean_path, sr=None)

        audio = add_jitter_delay(audio, sr)

        if rir_files:
            rir_path = np.random.choice(rir_files)
            rir_audio, _ = librosa.load(rir_path, sr=sr)
            audio = add_reverberation(audio, rir_audio)

        audio = add_white_noise(audio, target_snr)

        audio = audio / (np.max(np.abs(audio)) + 1e-10)

        filename = os.path.basename(clean_path)
        out_path = os.path.join(output_dir, f"noisy_{target_snr}dB_{filename}")
        sf.write(out_path, audio, sr)
        print(f"Сгенерирован: {out_path}")


if __name__ == "__main__":
    SRC_DIR = os.path.dirname(os.path.abspath(__file__))
    AI_RESEARCH_DIR = os.path.dirname(SRC_DIR)

    CLEAN_DIR = os.path.join(AI_RESEARCH_DIR, "data", "clean")
    RIR_DIR = os.path.join(AI_RESEARCH_DIR, "data", "rir")
    OUTPUT_DIR = os.path.join(AI_RESEARCH_DIR, "data", "noisy")

    print("Запуск пайплайна...")
    for snr in [20, 15, 10, 5]:
        process_pipeline(CLEAN_DIR, RIR_DIR, OUTPUT_DIR, target_snr=snr)
    print("Генерация завершена.")