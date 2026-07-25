package com.example.myapplication

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat

// Импортируем вашу тему и новые семантические цвета
import com.example.myapplication.ui.theme.MyApplicationTheme
import com.example.myapplication.ui.theme.TransferActive
import com.example.myapplication.ui.theme.TransferComplete
import com.example.myapplication.ui.theme.TransferError

// Заглушка для NativeModem, так как она не предоставлена в контексте
object NativeModem {
    fun startListening(): Boolean = true
    fun stopListening(): Boolean = true
}

// Заглушка для NativeModemStatus
object NativeModemStatus {
    const val TransferActive = 0
    const val TransferComplete = 1
    const val TransferError = 2
}

// Вспомогательная функция для применения модификаторов на основе условия
fun Modifier.ifTrue(condition: Boolean, modifier: Modifier.() -> Modifier): Modifier {
    return if (condition) {
        this.then(modifier())
    } else {
        this
    }
}

fun getBytesFromUri(context: Context, uri: Uri): ByteArray? {
    return try {
        context.contentResolver.openInputStream(uri)?.use { inputStream ->
            inputStream.readBytes()
        }
    } catch (e: Exception) {
        e.printStackTrace()
        null
    }
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MyApplicationTheme {
                ModemScreen()
            }
        }
    }
}

// Адаптивная карта передачи
@Composable
fun TransferCard(
    label: String,
    modifier: Modifier = Modifier,
    color: Color,
    icon: @Composable (modifier: Modifier) -> Unit,
    onClick: () -> Unit
) {
    val isDark = isSystemInDarkTheme()
    val shape = RoundedCornerShape(24.dp)

    // Рамка применяется всегда, а легкий фон — только в светлой теме
    val cardModifier = modifier
        .clickable(onClick = onClick)
        .background(
            color = if (!isDark) color.copy(alpha = 0.05f) else Color.Transparent,
            shape = shape
        )
        .border(1.dp, color = color, shape = shape)
        .padding(2.dp)

    Box(
        modifier = cardModifier
            .fillMaxWidth(0.55f)
            .aspectRatio(1f),
        contentAlignment = Alignment.Center
    ) {
        Column(
            modifier = Modifier.padding(32.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            icon(Modifier.size(64.dp))
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                text = label,
                color = if (!isDark) MaterialTheme.colorScheme.onBackground else color,
                style = MaterialTheme.typography.titleLarge,
                textAlign = TextAlign.Center
            )
        }
    }
}

@Composable
fun ModemScreen() {
    val context = LocalContext.current
    val isDark = isSystemInDarkTheme()

    var hasMicPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.RECORD_AUDIO
            ) == PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestPermission()
    ) { isGranted: Boolean ->
        hasMicPermission = isGranted
    }

    var engineStatus by remember { mutableStateOf("Ожидание запуска...") }
    var selectedFileUri by remember { mutableStateOf<Uri?>(null) }
    var fileBytes by remember { mutableStateOf<ByteArray?>(null) }

    var isListening by remember { mutableStateOf(false) }

    val filePickerLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        selectedFileUri = uri
        if (uri != null) {
            fileBytes = getBytesFromUri(context, uri)
        } else {
            fileBytes = null
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(MaterialTheme.colorScheme.background)
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {

        // --- Центральный адаптивный блок ---
        if (hasMicPermission) {
            // Весь этот Column занимает доступное пространство и центрирует контент
            Column(
                modifier = Modifier.weight(1f),
                verticalArrangement = Arrangement.Center
            ) {
                // Адаптивная Row 1 (Отправить) - УБРАН .weight(1f)
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.Center
                ) {
                    TransferCard(
                        label = "Отправить",
                        color = TransferActive, // Неоново-зеленый
                        icon = { modifier ->
                            Icon(
                                imageVector = Icons.Default.KeyboardArrowUp,
                                contentDescription = "Отправить",
                                modifier = modifier,
                                tint = TransferActive
                            )
                        },
                        onClick = { filePickerLauncher.launch("*/*") }
                    )
                }

                // Можно немного увеличить отступ между квадратными кнопками,
                // чтобы смотрелось гармоничнее
                Spacer(modifier = Modifier.height(48.dp))

                // Адаптивная Row 2 (Получить/Стоп) - УБРАН .weight(1f)
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.Center
                ) {
                    val label = if (isListening) "Стоп" else "Получить"
                    val color = if (isListening) TransferError else TransferComplete // Красный или фиолетовый

                    TransferCard(
                        label = label,
                        color = color,
                        icon = { modifier ->
                            Icon(
                                imageVector = Icons.Default.KeyboardArrowDown,
                                contentDescription = label,
                                modifier = modifier,
                                tint = color
                            )
                        },
                        onClick = {
                            if (isListening) {
                                val success = NativeModem.stopListening()
                                if (success) {
                                    isListening = false
                                    engineStatus = "Прослушивание остановлено"
                                } else {
                                    engineStatus = "Ошибка остановки"
                                }
                            } else {
                                val success = NativeModem.startListening()
                                if (success) {
                                    isListening = true
                                    engineStatus = "Эфир прослушивается..."
                                } else {
                                    engineStatus = "Ошибка старта микрофона"
                                }
                            }
                        }
                    )
                }
            }
        } else {
            // Блок разрешения микрофона (если нет разрешения)
            Column(
                modifier = Modifier.weight(1f),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Text(
                    text = "Для приема данных нужен доступ к микрофону",
                    color = TransferError, // Используем ваш цвет ошибки
                    style = MaterialTheme.typography.bodyMedium,
                    textAlign = TextAlign.Center
                )
                Spacer(modifier = Modifier.height(16.dp))
                Button(
                    onClick = { permissionLauncher.launch(Manifest.permission.RECORD_AUDIO) },
                    modifier = Modifier.fillMaxWidth(0.8f).height(64.dp), // Крупная кнопка
                    colors = ButtonDefaults.buttonColors(
                        containerColor = TransferError,
                        contentColor = MaterialTheme.colorScheme.onError
                    ),
                    shape = RoundedCornerShape(16.dp)
                ) {
                    Text("Разрешить микрофон", style = MaterialTheme.typography.titleMedium)
                }
            }
        }

        // --- Нижний неадаптивный блок ---
        // Занимает мало места
        Spacer(modifier = Modifier.height(16.dp))
        Text(
            text = engineStatus,
            color = MaterialTheme.colorScheme.onBackground,
            style = MaterialTheme.typography.bodyMedium, // Компактный текст
            textAlign = TextAlign.Center
        )
    }
}