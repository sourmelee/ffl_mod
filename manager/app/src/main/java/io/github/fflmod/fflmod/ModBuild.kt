package io.github.fflmod.fflmod

import android.content.Context
import android.os.Build
import android.os.StatFs
import io.github.fflmod.fflmod.core.ApkBuilder
import io.github.fflmod.fflmod.core.AxmlPatch
import io.github.fflmod.fflmod.core.Signer
import io.github.fflmod.fflmod.core.ZipRaw
import java.io.File
import java.io.IOException

/**
 * 入っている正規版と Release の差分一式から、fflmod の APK 一式を作る（入れるのは Installer）。
 * 出来上がりは アプリ専用のフォルダ/build（Android/data/io.github.fflmod.fflmod/files/build）に置く。
 */
object ModBuild {
    class Result(val dir: File, val version: String, val summaries: List<String>)

    /** 作業に要る空き容量の余裕 */
    private const val MARGIN = 50L shl 20

    fun run(ctx: Context, log: (String) -> Unit): Result {
        val st = Game.detect(ctx.packageManager)
        if (st !is GameState.Original || !st.ok) throw IOException(Texts.get(R.string.manager_24e1a87354))
        val apks = Game.apkFiles(ctx.packageManager)
        val out = File(ctx.getExternalFilesDir(null), "build")
        out.deleteRecursively()
        if (!out.mkdirs()) throw IOException(Texts.get(R.string.manager_35278869c4))
        // 作る間は署名済みの一式＋作業中の未署名のファイル一つ、入れるときは署名済みの一式＋インストールの作業領域への写しが要る
        val need = 2 * apks.sumOf { it.length() } + MARGIN
        val free = StatFs(out.path).availableBytes
        if (free < need) throw IOException(Texts.get(R.string.manager_bbbe8663a1, (need - free + (1 shl 20) - 1) shr 20))

        log(Texts.get(R.string.manager_ff84efdff8))
        val p = Release.fetchLatest(log)

        val base = apks.first()
        val man = ZipRaw.Reader(base).use { r ->
            r.read(r.find("AndroidManifest.xml") ?: throw IOException(Texts.get(R.string.manager_97695ec2a8)))
        }
        if (Release.sha256(man) != p.manifestFrom) throw IOException(Texts.get(R.string.manager_c9ad8de8de))
        val man2 = AxmlPatch.patch(man)
        if (Release.sha256(man2) != p.manifestTo) throw IOException(Texts.get(R.string.manager_ba051593d6))

        val signer = ctx.assets.open("fflmod_cert.der").use { cert ->
            Signer(ctx.assets.open("fflmod_key.pk8").use { it.readBytes() }, cert)
        }
        val v1 = Build.VERSION.SDK_INT >= 26
        val tmp = File(out, "unsigned.tmp")
        val summaries = ArrayList<String>()
        for (f in apks) {
            val name = f.name
            log(Texts.get(R.string.manager_8273d32799, name))
            when {
                f == base -> ApkBuilder.buildBase(f, man2, p.dex, tmp)
                name == "split_config.arm64_v8a.apk" -> ApkBuilder.buildArm64(f, p.so, tmp)
                else -> ApkBuilder.resignCopy(f, tmp)
            }
            val dst = File(out, name)
            signer.sign(tmp, dst, v1)
            tmp.delete()
            if (f == base || name == "split_config.arm64_v8a.apk") {
                val s = "$name ${ApkBuilder.summary(dst)}"
                summaries.add(s)
                log(s)
            }
        }
        File(out, "report.txt").writeText(
            "fflmod ${p.version} (${p.tag})\nsigning v1=$v1 v2=true\n" + summaries.joinToString("\n") + "\n"
        )
        return Result(out, p.version, summaries)
    }

    /** 更新で manifest から作れないとき（manifest の書き換えが版によって変わった） */
    class NeedReinstall : IOException(Texts.get(R.string.manager_2d3176180b))

    /** 更新：入っている mod の base・arm64 の中の mod のファイルだけを差し替えて署名する。ほかの分割ファイルはそのまま使う。
     *  戻り値は入れるファイルの一覧（作ったものと、入っている分割ファイル） */
    fun update(ctx: Context, p: Release.Patch, log: (String) -> Unit): Pair<Result, List<File>> {
        val st = Game.detect(ctx.packageManager)
        if (st !is GameState.Modded) throw IOException(Texts.get(R.string.manager_9aff273425))
        val apks = Game.apkFiles(ctx.packageManager)
        val base = apks.first()
        val arm64 = apks.firstOrNull { it.name == "split_config.arm64_v8a.apk" }
            ?: throw IOException(Texts.get(R.string.manager_e03e2751c1))
        val out = File(ctx.getExternalFilesDir(null), "build")
        out.deleteRecursively()
        if (!out.mkdirs()) throw IOException(Texts.get(R.string.manager_35278869c4))
        // 作るのは base と arm64 だけ。入れるときに一式がインストールの作業領域へ写される
        val need = 2 * (base.length() + arm64.length()) + apks.sumOf { it.length() } + MARGIN
        val free = StatFs(out.path).availableBytes
        if (free < need) throw IOException(Texts.get(R.string.manager_bbbe8663a1, (need - free + (1 shl 20) - 1) shr 20))

        var dexName: String? = null
        ZipRaw.Reader(base).use { r ->
            val man = r.read(r.find("AndroidManifest.xml") ?: throw IOException(Texts.get(R.string.manager_97695ec2a8)))
            if (Release.sha256(man) != p.manifestTo) throw NeedReinstall()
            val mark = "FFLModApp".toByteArray()
            for (e in r.entries) {
                if (!e.name.matches(Regex("classes[2-9][0-9]*\\.dex"))) continue
                if (indexOf(r.read(e), mark) >= 0) dexName = e.name
            }
        }
        val dn = dexName ?: throw IOException(Texts.get(R.string.manager_a7a85c0749))

        val signer = ctx.assets.open("fflmod_cert.der").use { cert ->
            Signer(ctx.assets.open("fflmod_key.pk8").use { it.readBytes() }, cert)
        }
        val v1 = Build.VERSION.SDK_INT >= 26
        val tmp = File(out, "unsigned.tmp")
        val summaries = ArrayList<String>()
        val made = ArrayList<File>()
        for ((src, name, data) in listOf(Triple(base, dn, p.dex), Triple(arm64, "lib/arm64-v8a/libfflmod.so", p.so))) {
            log(Texts.get(R.string.manager_f39ec316f0, src.name))
            ApkBuilder.rebuild(src, tmp, name, data, null, null, 0)
            val dst = File(out, src.name)
            signer.sign(tmp, dst, v1)
            tmp.delete()
            val s = "${src.name} ${ApkBuilder.summary(dst)}"
            summaries.add(s)
            log(s)
            made.add(dst)
        }
        File(out, "report.txt").writeText(
            "fflmod ${p.version} (${p.tag}) update\nsigning v1=$v1 v2=true\n" + summaries.joinToString("\n") + "\n"
        )
        val list = ArrayList<File>(made)
        for (f in apks) if (f != base && f != arm64) list.add(f)
        return Pair(Result(out, p.version, summaries), list)
    }

    private fun indexOf(h: ByteArray, n: ByteArray): Int {
        outer@ for (i in 0..h.size - n.size) {
            for (j in n.indices) if (h[i + j] != n[j]) continue@outer
            return i
        }
        return -1
    }
}
