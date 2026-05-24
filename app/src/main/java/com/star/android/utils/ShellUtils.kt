package com.star.android.utils

import android.content.Context
import android.util.Log
import java.io.DataOutputStream
import java.io.File
import java.io.FileOutputStream

object ShellUtils {
    private const val TAG = "StarShell"

    fun runAsRoot(command: String): Boolean {
        Log.d(TAG, "Menjalankan perintah root: $command")
        var process: Process? = null
        var os: DataOutputStream? = null
        return try {
            process = Runtime.getRuntime().exec("su")
            os = DataOutputStream(process.outputStream)
            
            os.writeBytes(command + "\n")
            os.writeBytes("exit\n")
            os.flush()

            val stdout = process.inputStream.bufferedReader().use { it.readText() }
            val stderr = process.errorStream.bufferedReader().use { it.readText() }
            val result = process.waitFor()
            
            if (stdout.isNotBlank()) Log.d(TAG, "STDOUT: $stdout")
            if (stderr.isNotBlank()) Log.e(TAG, "STDERR: $stderr")
            Log.d(TAG, "Exit Code: $result")

            val outputLower = stdout.lowercase()
            val isSuccess = result == 0 || outputLower.contains("success") || outputLower.contains("injected")
            
            isSuccess
        } catch (e: Exception) {
            Log.e(TAG, "Kesalahan sistem root: ${e.message}")
            false
        } finally {
            try { os?.close() } catch (e: Exception) {}
            process?.destroy()
        }
    }

    fun deployAndRunInjector(context: Context, packageName: String, localLibPath: String): Boolean {
        val injectorName = "Injector"
        val tempInjector = File(context.cacheDir, injectorName)

        return try {
            if (!File(localLibPath).exists()) {
                Log.e(TAG, "Library tidak ditemukan di path: $localLibPath")
                return false
            }

            context.assets.open(injectorName).use { input ->
                FileOutputStream(tempInjector).use { output ->
                    input.copyTo(output)
                }
            }

            val remoteLibPath = "/data/local/tmp/libStarcool.so"
            val remoteInjectorPath = "/data/local/tmp/$injectorName"

            val commandChain = "setenforce 0 ; " +
                    "rm -f \"$remoteInjectorPath\" ; " +
                    "rm -f \"$remoteLibPath\" ; " +
                    "cat \"${tempInjector.absolutePath}\" > \"$remoteInjectorPath\" ; " +
                    "cat \"$localLibPath\" > \"$remoteLibPath\" ; " +
                    "chmod 777 \"$remoteInjectorPath\" ; " +
                    "chmod 777 \"$remoteLibPath\" ; " +
                    "sync ; " +
                    "\"$remoteInjectorPath\" -pkg $packageName -lib \"$remoteLibPath\" -dl_memfd"

            runAsRoot(commandChain)
        } catch (e: Exception) {
            Log.e(TAG, "Deployment atau Injeksi gagal: ${e.message}")
            false
        }
    }
}
