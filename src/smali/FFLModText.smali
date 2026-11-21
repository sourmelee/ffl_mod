.class public final Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;
.super Ljava/lang/Object;
.source "FFLModText.java"

# Prefer the running game's language, then the manager's explicit language,
# then Android's locale. Native state is only sampled on the game thread.
.method private static japanese(Landroid/app/Activity;)Z
    .locals 3
    :try_start
    invoke-static {}, Lcom/square_enix/android_googleplay/ffl_gp/FFLMod;->nativeLanguage()I
    move-result v0
    if-ltz v0, :fallback
    if-eqz v0, :yes
    const/4 v0, 0x0
    return v0
    :try_end
    .catch Ljava/lang/UnsatisfiedLinkError; {:try_start .. :try_end} :no_native
    :no_native
    move-exception v0
    :fallback
    invoke-virtual {p0}, Landroid/app/Activity;->getIntent()Landroid/content/Intent;
    move-result-object v0
    if-eqz v0, :locale
    const-string v1, "fflmod.language"
    invoke-virtual {v0, v1}, Landroid/content/Intent;->getStringExtra(Ljava/lang/String;)Ljava/lang/String;
    move-result-object v0
    if-nez v0, :compare
    :locale
    invoke-static {}, Ljava/util/Locale;->getDefault()Ljava/util/Locale;
    move-result-object v0
    invoke-virtual {v0}, Ljava/util/Locale;->getLanguage()Ljava/lang/String;
    move-result-object v0
    :compare
    const-string v1, "ja"
    invoke-virtual {v1, v0}, Ljava/lang/String;->equals(Ljava/lang/Object;)Z
    move-result v0
    return v0
    :yes
    const/4 v0, 0x1
    return v0
.end method

.method public static save_dialog_4d7c60b538(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "セーブデータが見つかりません。ゲームで一度セーブしてから書き出してください。"
    return-object v0
    :english
    const-string v0, "No save data found. Save in the game at least once before exporting."
    return-object v0
.end method

.method public static save_dialog_0f63abbcef(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "書き出しに失敗しました。保存先を変えてもう一度お試しください。"
    return-object v0
    :english
    const-string v0, "Export failed. Choose a different location and try again."
    return-object v0
.end method

.method public static save_dialog_c181bb0788(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "セーブデータを書き出しました。"
    return-object v0
    :english
    const-string v0, "Save data exported."
    return-object v0
.end method

.method public static save_dialog_e1f2ac561c(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "ファイルを読み込めませんでした。"
    return-object v0
    :english
    const-string v0, "Could not read the file."
    return-object v0
.end method

.method public static save_dialog_4c0c678f5d(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "このファイルはFFレジェンズのセーブデータではありません。"
    return-object v0
    :english
    const-string v0, "This file is not FF Dimensions save data."
    return-object v0
.end method

.method public static save_dialog_7d0775b142(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "Google データ エクスポートのクラウドセーブ"
    return-object v0
    :english
    const-string v0, "The cloud save from Google Takeout"
    return-object v0
.end method

.method public static save_dialog_250bd3ad36(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "（最終更新："
    return-object v0
    :english
    const-string v0, " (Last updated: "
    return-object v0
.end method

.method public static save_dialog_597919fc78(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "を取り込みます。"
    return-object v0
    :english
    const-string v0, " will be imported."
    return-object v0
.end method

.method public static save_dialog_c585303770(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "このファイルにはセーブデータが入っていません。一緒に届いたもう一つのzipファイルを選んでください。"
    return-object v0
    :english
    const-string v0, "This file contains no save data. Select the other ZIP file included in your export."
    return-object v0
.end method

.method public static save_dialog_9c1dec2d3e(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "このzipファイルにはFFレジェンズのセーブデータが入っていません。"
    return-object v0
    :english
    const-string v0, "This ZIP file contains no FF Dimensions save data."
    return-object v0
.end method

.method public static save_dialog_9c7c18ecb5(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "選んだセーブデータを取り込みます。"
    return-object v0
    :english
    const-string v0, "The selected save data will be imported."
    return-object v0
.end method

.method public static save_dialog_2962a5338c(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "今のセーブデータは上書きされます。必要な場合は、先に書き出しておいてください。取り込んだあと、ゲームを終了します。"
    return-object v0
    :english
    const-string v0, "Your current save data will be overwritten. Export it first if you want to keep it. The game will close after the import."
    return-object v0
.end method

.method public static save_dialog_d535dca5e3(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "セーブデータの取り込み"
    return-object v0
    :english
    const-string v0, "Import save data"
    return-object v0
.end method

.method public static save_dialog_94ad857900(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "取り込む"
    return-object v0
    :english
    const-string v0, "Import"
    return-object v0
.end method

.method public static save_dialog_a56331c3d8(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "やめる"
    return-object v0
    :english
    const-string v0, "Cancel"
    return-object v0
.end method

.method public static save_dialog_78087f06a3(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "取り込みの準備ができました。ゲームを終了します。次にゲームを起動したときに反映されます。"
    return-object v0
    :english
    const-string v0, "The import is ready. The game will now close. Your save data will be applied the next time you start the game."
    return-object v0
.end method

.method public static save_dialog_a30871d8bb(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "取り込みに失敗しました。"
    return-object v0
    :english
    const-string v0, "Import failed."
    return-object v0
.end method

.method public static save_dialog_e532b24c6b(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "取り込みを取りやめました。"
    return-object v0
    :english
    const-string v0, "Import canceled."
    return-object v0
.end method

.method public static save_dialog_603127c025(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "書き出しを取りやめました。"
    return-object v0
    :english
    const-string v0, "Export canceled."
    return-object v0
.end method

.method public static date_close(Landroid/app/Activity;)Ljava/lang/String;
    .locals 1
    invoke-static {p0}, Lcom/square_enix/android_googleplay/ffl_gp/FFLModText;->japanese(Landroid/app/Activity;)Z
    move-result v0
    if-eqz v0, :english
    const-string v0, "）"
    return-object v0
    :english
    const-string v0, ")"
    return-object v0
.end method
