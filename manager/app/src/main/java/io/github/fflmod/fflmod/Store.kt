package io.github.fflmod.fflmod

import android.content.Context
import java.io.File

/**
 * 導入の途中の状態と、最後の結果の表示を覚えておく（ほかのアプリから戻ったときや、アプリが閉じられたあとも続けられるように）。
 * phase：""（何もしていない）、"uninstall"（APK を作り終え、正規版の削除を待っている）、"install"（インストールの確認を待っている）
 */
object Store {
    private fun sp(ctx: Context) = ctx.getSharedPreferences("fflmod", Context.MODE_PRIVATE)

    fun phase(ctx: Context): String = sp(ctx).getString("phase", "") ?: ""
    fun setPhase(ctx: Context, p: String) = sp(ctx).edit().putString("phase", p).apply()

    fun version(ctx: Context): String = sp(ctx).getString("version", "") ?: ""
    fun setVersion(ctx: Context, v: String) = sp(ctx).edit().putString("version", v).apply()

    fun message(ctx: Context): String = sp(ctx).getString("message", "") ?: ""
    fun log(ctx: Context): String = sp(ctx).getString("log", "") ?: ""
    fun setMessage(ctx: Context, message: String, log: String) =
        sp(ctx).edit().putString("message", message).putString("log", log).apply()

    /** 試験版（プレリリース）も受け取るか */
    fun testing(ctx: Context): Boolean = sp(ctx).getBoolean("testing", false)
    fun setTesting(ctx: Context, on: Boolean) = sp(ctx).edit().putBoolean("testing", on).apply()

    /** 作った APK 一式の場所 */
    fun buildDir(ctx: Context) = File(ctx.getExternalFilesDir(null), "build")

    /** 作り終えた一式（report.txt があるときだけ） */
    fun builtApks(ctx: Context): List<File>? {
        val d = buildDir(ctx)
        if (!File(d, "report.txt").isFile || !File(d, "base.apk").isFile) return null
        return d.listFiles { f -> f.name.endsWith(".apk") }?.sortedBy { if (it.name == "base.apk") "" else it.name }
    }

    /** "install"（正規版から導入）か "update"（mod の更新） */
    fun mode(ctx: Context): String = sp(ctx).getString("mode", "install") ?: "install"
    fun setMode(ctx: Context, m: String) = sp(ctx).edit().putString("mode", m).apply()

    fun finishInstall(ctx: Context, ok: Boolean, reason: String?) {
        setPhase(ctx, "")
        val update = mode(ctx) == "update"
        if (ok) {
            buildDir(ctx).deleteRecursively()
            setMessage(ctx, if (update) Texts.get(R.string.manager_6f92cf3c15, version(ctx))
                else Texts.get(R.string.manager_bd23c7dc4a, version(ctx)), "")
        } else {
            setMessage(ctx, if (update) Texts.get(R.string.manager_130897d40e) else Texts.get(R.string.manager_9c6a140bd3),
                reason ?: "")
        }
    }
}
