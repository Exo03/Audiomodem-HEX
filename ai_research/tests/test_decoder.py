import pytest
import numpy as np


def mock_cpp_decoder(audio_array, sample_rate):
    noise_level = np.mean(np.abs(audio_array))
    success_rate = max(0.0, 100.0 - (noise_level * 100))
    return success_rate


class TestAcousticModem:

    @pytest.mark.parametrize("snr_db", [25, 20, 15, 10, 5])
    def test_noise_resilience(self, snr_db):
        dummy_signal = np.random.normal(0, 10 ** (-snr_db / 20), 16000)
        success_rate = mock_cpp_decoder(dummy_signal, 16000)
        assert success_rate >= 80.0

    @pytest.mark.parametrize("jitter_ms", [0, 2, 10, 30])
    def test_sync_jitter(self, jitter_ms):
        delay_samples = int((jitter_ms / 1000.0) * 16000)
        dummy_signal = np.concatenate([np.zeros(delay_samples), np.ones(16000)])[:16000]
        success_rate = mock_cpp_decoder(dummy_signal, 16000)
        assert success_rate >= 75.0