import torch
import torch.nn as nn


class PhasePreservingDenoiser(nn.Module):

    def __init__(self):
        super(PhasePreservingDenoiser, self).__init__()

        self.encoder = nn.Sequential(
            nn.Conv1d(in_channels=1, out_channels=16, kernel_size=15, stride=1, padding=7),
            nn.PReLU(),
            nn.Conv1d(in_channels=16, out_channels=32, kernel_size=15, stride=1, padding=7),
            nn.PReLU()
        )

        self.decoder = nn.Sequential(
            nn.Conv1d(in_channels=32, out_channels=16, kernel_size=15, stride=1, padding=7),
            nn.PReLU(),
            nn.Conv1d(in_channels=16, out_channels=1, kernel_size=15, stride=1, padding=7),
            nn.Tanh()
        )

    def forward(self, x):

        encoded = self.encoder(x)
        decoded = self.decoder(encoded)
        return decoded


if __name__ == "__main__":
    model = PhasePreservingDenoiser()
    dummy_audio = torch.randn(1, 1, 16000)
    output = model(dummy_audio)
    print(f"Входной тензор: {dummy_audio.shape} -> Выходной тензор: {output.shape}")