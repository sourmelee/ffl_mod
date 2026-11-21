package io.github.fflmod.fflmod

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

/**
 * 管理アプリの画面（1画面）。
 * 「modを導入」で APK 一式を作り、正規版を削除して、mod の一式を入れる。「更新」は入っている mod の上から新しい版を入れる。
 * セーブの書き出し・取り込みはゲームの画面を開く。
 */
class MainActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var versions: TextView
    private lateinit var result: TextView
    private lateinit var log: TextView
    private lateinit var install: Button
    private lateinit var update: Button
    private lateinit var export: Button
    private lateinit var importBtn: Button
    private val main = Handler(Looper.getMainLooper())
    private var state: GameState? = null
    /** 最新の mod の版。null はまだ読んでいない、"" は確認できなかった */
    private var latest: String? = null
    private var busy = false
    private var waitingUninstall = false
    private val logBuf = StringBuilder()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val pad = (16 * resources.displayMetrics.density).toInt()
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
        }
        val lp = { LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT) }
        status = TextView(this).apply { textSize = 18f }
        root.addView(status, lp())
        versions = TextView(this).apply {
            textSize = 16f; setPadding(0, pad / 2, 0, 0)
            /* 長押しで試験版を受け取るかを切り替える（試験用。画面には出さない） */
            setOnLongClickListener { toggleTesting(); true }
        }
        root.addView(versions, lp())
        result = TextView(this).apply { textSize = 16f; setPadding(0, pad / 2, 0, pad / 2) }
        root.addView(result, lp())
        install = Button(this).apply { text = Texts.get(R.string.manager_2b226235e6); setOnClickListener { onInstall() } }
        root.addView(install, lp())
        update = Button(this).apply { text = Texts.get(R.string.manager_4d317c8ecb); setOnClickListener { onUpdate() } }
        root.addView(update, lp())
        export = Button(this).apply { text = Texts.get(R.string.manager_0c56af39c8); setOnClickListener { openSave("fflmod.intent.action.EXPORT_SAVE") } }
        root.addView(export, lp())
        importBtn = Button(this).apply { text = Texts.get(R.string.manager_70a602ed0c); setOnClickListener { openSave("fflmod.intent.action.IMPORT_SAVE") } }
        root.addView(importBtn, lp())
        /* ボタンの英字を大文字にしない（Android の既定は大文字にする） */
        for (b in listOf(install, update, export, importBtn)) b.isAllCaps = false
        log = TextView(this).apply { textSize = 12f; setTextIsSelectable(true) }
        root.addView(log, lp())
        setContentView(ScrollView(this).apply { addView(root) })
        Release.includePre = Store.testing(this)
        Installer.listener = { r -> main.post { onInstallResult(r) } }
    }

    override fun onDestroy() {
        Installer.listener = null
        super.onDestroy()
    }

    override fun onResume() {
        super.onResume()
        if (!busy) refresh()
    }

    // ---- 状態の表示 ----

    private fun refresh() {
        Thread {
            val st = detect()
            main.post { show(st) }
        }.start()
        Thread {
            val v = try { Release.latestVersion() } catch (e: Exception) { "" }
            main.post { latest = v; showVersions() }
        }.start()
    }

    private fun toggleTesting() {
        if (busy) return
        val on = !Store.testing(this)
        Store.setTesting(this, on)
        Release.includePre = on
        Toast.makeText(this, if (on) Texts.get(R.string.manager_5b0fb11e05) else Texts.get(R.string.manager_b96d1ec1fa), Toast.LENGTH_SHORT).show()
        latest = null
        showVersions()
        refresh()
    }

    /** 入っている mod と最新の mod の版を並べて出す */
    private fun showVersions() {
        val st = state
        val have = when (st) {
            is GameState.Modded -> st.version ?: Texts.get(R.string.manager_fc885bbb4d)
            null -> Texts.get(R.string.manager_23b9b1c924)
            else -> Texts.get(R.string.manager_44ff697ecd)
        }
        val l = latest
        val sb = StringBuilder(Texts.get(R.string.manager_54b1f3d50f)).append(have).append(Texts.get(R.string.manager_a98e734b90))
        when {
            l == null -> sb.append(Texts.get(R.string.manager_2c201069ad))
            l.isEmpty() -> sb.append(Texts.get(R.string.manager_8312c0f9f9))
            else -> {
                sb.append(l)
                if (Release.includePre) sb.append(Texts.get(R.string.manager_0fc0b5593c))
                if (st is GameState.Modded && st.version != null)
                    sb.append(if (st.version == l) Texts.get(R.string.manager_321771e517) else Texts.get(R.string.manager_35981549ed))
            }
        }
        versions.text = sb
    }

    private fun detect(): GameState? = try {
        Game.detect(packageManager)
    } catch (e: Exception) {
        null
    }

    private fun show(st: GameState?) {
        state = st
        status.text = when (st) {
            null -> Texts.get(R.string.manager_e5cd356be8)
            is GameState.NotInstalled -> Texts.get(R.string.manager_70dfd1dddb)
            is GameState.Original ->
                if (st.ok) Texts.get(R.string.manager_8cb1797535)
                else Texts.get(R.string.manager_40464cf82e)
            is GameState.Modded ->
                Texts.get(R.string.manager_3cc336541f)
            is GameState.Unknown -> Texts.get(R.string.manager_93769bcac1)
        }
        // 導入の途中から続ける（アプリが閉じられていたときなど）
        when (Store.phase(this)) {
            "uninstall" -> if (!waitingUninstall) {
                if (st is GameState.NotInstalled) { startInstall(); return }
                Store.setPhase(this, "")
                Store.setMessage(this, Texts.get(R.string.manager_483c1721e2), Store.log(this))
            }
            "perm" -> {
                Store.setPhase(this, "")
                if (st is GameState.NotInstalled && canInstall()) { startInstall(); return }
            }
            "install" -> if (st is GameState.Modded && (Store.mode(this) != "update" || st.version == Store.version(this)))
                Store.finishInstall(this, true, null)
        }
        showVersions()
        result.text = Store.message(this)
        if (!busy) log.text = Store.log(this).ifEmpty {
            when (st) {
                is GameState.Original -> st.detail
                is GameState.Unknown -> "cert ${st.certSha256}"
                else -> ""
            }
        }
        updateButtons()
    }

    private fun updateButtons() {
        val st = state
        install.isEnabled = !busy && ((st is GameState.Original && st.ok) ||
                (st is GameState.NotInstalled && Store.builtApks(this) != null))
        update.isEnabled = !busy && st is GameState.Modded
        export.isEnabled = !busy && st is GameState.Modded
        importBtn.isEnabled = !busy && st is GameState.Modded
    }

    private fun addLog(s: String) = main.post {
        logBuf.append(s).append('\n')
        log.text = logBuf
    }

    private fun setBusy(b: Boolean) {
        busy = b
        updateButtons()
    }

    // ---- 導入 ----

    private fun onInstall() {
        val st = state
        if (st is GameState.NotInstalled) {
            Store.setMode(this, "install")
            startInstall()   // 作った一式が残っている（前回の導入が途中で終わった）
            return
        }
        if (st !is GameState.Original || !st.ok) return
        setBusy(true)
        logBuf.setLength(0)
        log.text = ""
        result.text = ""
        status.text = Texts.get(R.string.manager_c457019e38)
        Thread {
            val r = try {
                ModBuild.run(this) { addLog(it) }
            } catch (e: Exception) {
                addLog(e.toString())
                null
            }
            main.post {
                setBusy(false)
                if (r == null) {
                    Store.setMessage(this, Texts.get(R.string.manager_c7e6687460), logBuf.toString())
                    refresh()
                } else {
                    Store.setVersion(this, r.version)
                    Store.setMode(this, "install")
                    Store.setMessage(this, "", logBuf.toString())
                    confirmUninstall()
                }
            }
        }.start()
    }

    // ---- 更新 ----

    private fun onUpdate() {
        val st = state as? GameState.Modded ?: return
        setBusy(true)
        logBuf.setLength(0)
        log.text = ""
        result.text = ""
        status.text = Texts.get(R.string.manager_6f02e58234)
        Thread {
            val p = try {
                Release.fetchLatest { addLog(it) }
            } catch (e: Exception) {
                addLog(e.toString())
                null
            }
            main.post {
                if (p == null) {
                    setBusy(false)
                    Store.setMessage(this, Texts.get(R.string.manager_130897d40e), logBuf.toString())
                    refresh()
                } else if (st.version == p.version) {
                    AlertDialog.Builder(this)
                        .setTitle(Texts.get(R.string.manager_109b75d0fa))
                        .setMessage(Texts.get(R.string.manager_a580145ee5, p.version))
                        .setPositiveButton(Texts.get(R.string.manager_1230dda998)) { _, _ -> buildUpdate(p) }
                        .setNegativeButton(Texts.get(R.string.manager_3eefdf40e6)) { _, _ -> setBusy(false); refresh() }
                        .setCancelable(false)
                        .show()
                } else {
                    buildUpdate(p)
                }
            }
        }.start()
    }

    private fun buildUpdate(p: Release.Patch) {
        Thread {
            var msg = Texts.get(R.string.manager_130897d40e)
            val r = try {
                ModBuild.update(this, p) { addLog(it) }
            } catch (e: ModBuild.NeedReinstall) {
                msg = Texts.get(R.string.manager_a33a1528a2)
                null
            } catch (e: Exception) {
                addLog(e.toString())
                null
            }
            main.post {
                setBusy(false)
                if (r == null) {
                    Store.setMessage(this, msg, logBuf.toString())
                    refresh()
                } else {
                    Store.setVersion(this, r.first.version)
                    Store.setMode(this, "update")
                    Store.setMessage(this, "", logBuf.toString())
                    startInstall(r.second)
                }
            }
        }.start()
    }

    private fun confirmUninstall() {
        AlertDialog.Builder(this)
            .setTitle(Texts.get(R.string.manager_a7372f32aa))
            .setMessage(Texts.get(R.string.manager_96d9f3a967))
            .setPositiveButton(Texts.get(R.string.manager_56b8d59b66)) { _, _ ->
                AlertDialog.Builder(this)
                    .setTitle(Texts.get(R.string.manager_3c072ca7b3))
                    .setMessage(Texts.get(R.string.manager_abfd1659d9))
                    .setPositiveButton(Texts.get(R.string.manager_f17ccd9811)) { _, _ -> uninstall() }
                    .setNegativeButton(Texts.get(R.string.manager_3eefdf40e6)) { _, _ -> refresh() }
                    .setCancelable(false)
                    .show()
            }
            .setNegativeButton(Texts.get(R.string.manager_3eefdf40e6)) { _, _ -> refresh() }
            .setCancelable(false)
            .show()
    }

    @Suppress("DEPRECATION")
    private fun uninstall() {
        Store.setPhase(this, "uninstall")
        waitingUninstall = true
        val i = Intent(Intent.ACTION_UNINSTALL_PACKAGE, Uri.parse("package:" + Game.PKG))
            .putExtra(Intent.EXTRA_RETURN_RESULT, true)
        startActivityForResult(i, REQ_UNINSTALL)
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQ_UNINSTALL) return
        setBusy(true)
        // 削除の画面が閉じてから、削除が終わるまで少し待つことがある
        Thread {
            var st = detect()
            var n = 0
            while (st !is GameState.NotInstalled && n++ < 10) {
                Thread.sleep(500)
                st = detect()
            }
            main.post {
                waitingUninstall = false
                setBusy(false)
                if (st is GameState.NotInstalled) {
                    state = st
                    startInstall()
                } else {
                    Store.setPhase(this, "")
                    Store.setMessage(this, Texts.get(R.string.manager_483c1721e2), Store.log(this))
                    refresh()
                }
            }
        }.start()
    }

    private fun canInstall() = Build.VERSION.SDK_INT < 26 || packageManager.canRequestPackageInstalls()

    private fun startInstall(list: List<java.io.File>? = null) {
        val apks = list ?: Store.builtApks(this)
        if (apks == null) {
            Store.setPhase(this, "")
            Store.setMessage(this, Texts.get(R.string.manager_9c6a140bd3), Texts.get(R.string.manager_e216616b97))
            refresh()
            return
        }
        if (!canInstall()) {
            Store.setPhase(this, "perm")
            Store.setMessage(this, Texts.get(R.string.manager_05bd258fe5), Store.log(this))
            result.text = Store.message(this)
            startActivity(Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:$packageName")))
            return
        }
        Store.setPhase(this, "install")
        setBusy(true)
        status.text = if (Store.mode(this) == "update") Texts.get(R.string.manager_81ddfa9624) else Texts.get(R.string.manager_f568d6d810)
        Thread {
            try {
                Installer.install(this, apks)
            } catch (e: Exception) {
                main.post { onInstallResult(Installer.Result.Failure(e.toString())) }
            }
        }.start()
    }

    private fun onInstallResult(r: Installer.Result) {
        when (r) {
            is Installer.Result.NeedUser -> {
                startActivity(r.intent)
                return
            }
            is Installer.Result.Success -> Store.finishInstall(this, true, null)
            is Installer.Result.Failure -> Store.finishInstall(this, false, r.message)
        }
        setBusy(false)
        refresh()
    }

    // ---- セーブ ----

    private fun openSave(action: String) {
        try {
            startActivity(Intent(action).setClassName(Game.PKG, Game.PKG + ".FFLModSaveActivity").putExtra("fflmod.language", Texts.language()))
        } catch (e: Exception) {
            result.text = Texts.get(R.string.manager_fcf6e26970)
            log.text = e.toString()
        }
    }

    companion object {
        private const val REQ_UNINSTALL = 1
    }
}
