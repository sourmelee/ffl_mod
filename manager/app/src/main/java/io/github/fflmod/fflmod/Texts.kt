package io.github.fflmod.fflmod

import android.app.Application
import android.content.Context

/** Resources are available to background download/build errors as well as UI. */
object Texts {
    private lateinit var context: Context

    fun init(app: Context) { context = app.applicationContext }

    fun get(id: Int, vararg args: Any): String =
        if (args.isEmpty()) context.getString(id) else context.getString(id, *args)

    fun language(): String =
        if (context.resources.configuration.locales[0].language == "ja") "ja" else "en"
}

class ModApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        Texts.init(this)
    }
}
