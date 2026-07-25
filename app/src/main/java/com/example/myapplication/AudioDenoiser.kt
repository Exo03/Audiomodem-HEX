package com.example.myapplication

import android.content.Context
import org.pytorch.IValue
import org.pytorch.Module
import org.pytorch.Tensor
import org.pytorch.LiteModuleLoader
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

class AudioDenoiser(private val context: Context) {

    // Ленивая загрузка модели при первом обращении
    private val module: Module by lazy {
        val modelPath = getAssetFilePath("denoiser_mobile.ptl")

        LiteModuleLoader.load(modelPath)
    }

    fun denoiseFile(inputPath: String, outputPath: String): Boolean {
        return try {
            val fileBytes = File(inputPath).readBytes()

            // WAV-заголовок занимает первые 44 байта, отделяем его от сырых данных
            val header = fileBytes.copyOfRange(0, 44)
            val audioBytes = fileBytes.copyOfRange(44, fileBytes.size)

            // Конвертируем 16-битный звук (short) в FloatArray (от -1.0 до 1.0)
            val shortsCount = audioBytes.size / 2
            val floatArray = FloatArray(shortsCount)
            val byteBuffer = ByteBuffer.wrap(audioBytes).order(ByteOrder.LITTLE_ENDIAN)
            for (i in 0 until shortsCount) {
                floatArray[i] = byteBuffer.short / 32768.0f
            }

            // Создаем PyTorch Тензор размерности [1, 1, количество_сэмплов]
            val inputTensor = Tensor.fromBlob(floatArray, longArrayOf(1, 1, floatArray.size.toLong()))

            // 🚀 ПРОГОН ЧЕРЕЗ НЕЙРОСЕТЬ
            val outputTensor = module.forward(IValue.from(inputTensor)).toTensor()
            val outputFloatArray = outputTensor.dataAsFloatArray

            // Конвертируем обратно из Float в 16-битный short
            val outputByteBuffer = ByteBuffer.allocate(outputFloatArray.size * 2).order(ByteOrder.LITTLE_ENDIAN)
            for (f in outputFloatArray) {
                // Защита от перегрузки (клиппинга)
                var value = (f * 32768.0f).toInt()
                if (value > 32767) value = 32767
                if (value < -32768) value = -32768
                outputByteBuffer.putShort(value.toShort())
            }

            // Собираем новый очищенный WAV-файл
            val outputFile = File(outputPath)
            outputFile.writeBytes(header) // Возвращаем заголовок
            outputFile.appendBytes(outputByteBuffer.array()) // Дописываем чистый звук

            true
        } catch (e: Exception) {
            e.printStackTrace()
            false
        }
    }

    // Вспомогательная функция для копирования файла из assets во внутреннюю память
    private fun getAssetFilePath(assetName: String): String {
        val file = File(context.filesDir, assetName)
        if (file.exists() && file.length() > 0) return file.absolutePath

        context.assets.open(assetName).use { inputStream ->
            FileOutputStream(file).use { outputStream ->
                val buffer = ByteArray(4 * 1024)
                var read: Int
                while (inputStream.read(buffer).also { read = it } != -1) {
                    outputStream.write(buffer, 0, read)
                }
                outputStream.flush()
            }
        }
        return file.absolutePath
    }
}