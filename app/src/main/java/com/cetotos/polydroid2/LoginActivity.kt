package com.cetotos.polydroid2

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.widget.LinearLayout
import android.widget.ProgressBar
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import kotlin.concurrent.thread

class LoginActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "PolyDroid2"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (!handlePolytoriaIntent(intent)) {
            finish()
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        handlePolytoriaIntent(intent)
    }

    private fun handlePolytoriaIntent(intent: Intent): Boolean {
        val uri = intent.data ?: return false
        if (uri.scheme != "luduvo") return false
        handlePolytoriaUri(uri)
        return true
    }

    // luduvo://<token>/<host>/<port>[/<server_pubkey_hex>] is passed straight to LuduvoGame
    private fun handlePolytoriaUri(uri: Uri) {
        val raw = uri.toString()
        val segments = raw.substringAfter("://").substringBefore('?').substringBefore('#')
            .split('/').filter { it.isNotBlank() }

        if (segments.isEmpty()) {
            Log.e(TAG, "Invalid URL!")
            finish()
            return
        }
        Log.i(TAG, "luduvo link, first segment=${segments[0].take(8)}..., parts=${segments.size}")

        // single-quote for the guest sh script, strip anything that could break out
        val safe = raw.filter { it.isLetterOrDigit() || it in ":/._-?=&%+~@" }
        val execArgs = "'$safe'"
        prepareClientThenLaunch(segments[0], ClientDownloader.Channel.STABLE, execArgs)
    }

    private fun prepareClientThenLaunch(token: String, channel: ClientDownloader.Channel, execArgs: String) {
        val bar = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            isIndeterminate = false
            max = 100
            progress = 0
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
        }
        val dialogView = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(64, 48, 64, 48)
            addView(bar)
        }
        val dialog = MaterialAlertDialogBuilder(this)
            .setTitle("Preparing ${channel.label} client…")
            .setView(dialogView)
            .setCancelable(false)
            .create()
        dialog.setCanceledOnTouchOutside(false)
        dialog.show()

        thread {
            try {
                if (RootFs.needsExtraction(this)) {
                    RootFs.extractAll(this) { pct, _, _, label ->
                        runOnUiThread {
                            dialog.setTitle(label)
                            bar.progress = pct
                        }
                    }
                }
                ClientDownloader.prepare(this, token, channel) { pct, label ->
                    runOnUiThread {
                        dialog.setTitle(label)
                        bar.progress = pct
                    }
                }
                runOnUiThread {
                    if (isFinishing || isDestroyed) return@runOnUiThread
                    dialog.dismiss()
                    startActivity(Intent(this, GameActivity::class.java).apply {
                        putExtra("exec_args", execArgs)
                    })
                    finish()
                }
            } catch (e: Exception) {
                Log.e(TAG, "client prepare failed: ${e.message}", e)
                runOnUiThread {
                    if (isFinishing || isDestroyed) return@runOnUiThread
                    dialog.dismiss()
                    MaterialAlertDialogBuilder(this)
                        .setTitle("Couldn't get the ${channel.label} client")
                        .setMessage("${e.message}\n\nPress Play again to retry.")
                        .setPositiveButton("OK") { _, _ -> finish() }
                        .setOnDismissListener { finish() }
                        .show()
                }
            }
        }
    }
}
