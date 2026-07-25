import torch
from model import PhasePreservingDenoiser

# 1. Инициализируем модель и загружаем веса
model = PhasePreservingDenoiser()
model.load_state_dict(torch.load("denoiser.pth", map_location="cpu"))
model.eval()

# 2. Создаем тестовый тензор
example_input = torch.randn(1, 1, 44100)

# 3. Трассируем граф вычислений нейросети
traced_script_module = torch.jit.trace(model, example_input)

# 4. Сохраняем напрямую, МИНУЯ функцию optimize_for_mobile
traced_script_module._save_for_lite_interpreter("denoiser_mobile.ptl")

print("Модель успешно экспортирована в denoiser_mobile.ptl (без XNNPACK)")