import os
import glob
import torch
import torch.nn as nn
import torch.optim as optim
import torchaudio
from model import PhasePreservingDenoiser


def load_dataset(clean_dir, noisy_dir, target_length=44100 * 2):
    clean_files = sorted(glob.glob(os.path.join(clean_dir, "*.wav")))
    noisy_files = sorted(glob.glob(os.path.join(noisy_dir, "*.wav")))

    x_data, y_data = [], []
    for clean_path, noisy_path in zip(clean_files, noisy_files):
        clean_wav, _ = torchaudio.load(clean_path)
        noisy_wav, _ = torchaudio.load(noisy_path)

        if clean_wav.shape[1] > target_length:
            clean_wav = clean_wav[:, :target_length]
            noisy_wav = noisy_wav[:, :target_length]
        else:
            padding = target_length - clean_wav.shape[1]
            clean_wav = torch.nn.functional.pad(clean_wav, (0, padding))
            noisy_wav = torch.nn.functional.pad(noisy_wav, (0, padding))

        x_data.append(noisy_wav)
        y_data.append(clean_wav)

    return torch.stack(x_data), torch.stack(y_data)


def train_model():
    SRC_DIR = os.path.dirname(os.path.abspath(__file__))
    AI_RESEARCH_DIR = os.path.dirname(SRC_DIR)
    clean_dir = os.path.join(AI_RESEARCH_DIR, "data", "clean")
    noisy_dir = os.path.join(AI_RESEARCH_DIR, "data", "noisy")

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = PhasePreservingDenoiser().to(device)
    criterion = nn.L1Loss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)

    X_train, Y_train = load_dataset(clean_dir, noisy_dir)
    X_train, Y_train = X_train.to(device), Y_train.to(device)

    epochs = 50
    for epoch in range(epochs):
        model.train()
        optimizer.zero_grad()

        outputs = model(X_train)
        loss = criterion(outputs, Y_train)
        loss.backward()
        optimizer.step()

    torch.save(model.state_dict(), os.path.join(SRC_DIR, "denoiser.pth"))


if __name__ == "__main__":
    train_model()