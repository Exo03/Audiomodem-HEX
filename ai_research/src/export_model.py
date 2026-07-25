import torch
from torch.utils.mobile_optimizer import optimize_for_mobile
from model import PhasePreservingDenoiser

# 1. Инициализируем модель и загружаем веса
model = PhasePreservingDenoiser()
model.load_state_dict(torch.load("denoiser.pth", map_location="cpu"))
model.eval() # Обязательно переводим в режим инференса

# 2. Создаем тестовый тензор (формат: batch, channels, length)
# Раз нейросеть обучалась на 1-канальном звуке, используем 1 канал
example_input = torch.randn(1, 1, 44100)

# 3. Трассируем граф вычислений нейросети
traced_script_module = torch.jit.trace(model, example_input)

# 4. Оптимизируем для мобилок и сохраняем
optimized_scripted_module = optimize_for_mobile(traced_script_module)
optimized_scripted_module._save_for_lite_interpreter("denoiser_mobile.ptl")

print("Модель успешно экспортирована в denoiser_mobile.ptl")