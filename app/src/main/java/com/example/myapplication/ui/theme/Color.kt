package com.example.myapplication.ui.theme

import androidx.compose.ui.graphics.Color

// Базовые цвета логотипа (Светящиеся/Неоновые волны)
val GreenNeon = Color(0xFF3DE684)
val PurpleLight = Color(0xFFB19BFF)
val PinkNeon = Color(0xFFFF7DF3)

// Адаптированные цвета для светлой темы
val GreenDark = Color(0xFF006D3A)
val PurpleDark = Color(0xFF5A46B5)
val PinkDark = Color(0xFF9B0091)

// Фоны приложения
val BackgroundDark = Color(0xFF23252A)
val SurfaceDark = Color(0xFF2E3036)
val BackgroundLight = Color(0xFFFBFDF8)
val SurfaceLight = Color(0xFFF2F4F0)

// Дополнительные семантические цвета для передачи файлов по звуку
val TransferActive = GreenNeon      // Активная передача (пульсация)
val TransferError = Color(0xFFFF4C4C) // Ошибка / Помехи (красный)
val TransferComplete = PurpleLight    // Успешное завершение
val SignalWaveform = Color(0x803DE684) // Полупрозрачный для фоновых волн