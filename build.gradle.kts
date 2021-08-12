plugins {
    kotlin("jvm") version "1.4.30"
    kotlin("plugin.serialization") version "1.4.30"

    id("net.mamoe.mirai-console") version "2.6.5"
}

group = "org.itxtech"
version = "2.0.2-cp"
description = "强大的 mirai 原生插件加载器。"

kotlin {
    sourceSets {
        all {
            languageSettings.enableLanguageFeature("InlineClasses")
            languageSettings.useExperimentalAnnotation("kotlin.Experimental")
        }
    }
}

repositories {
    mavenCentral()
    maven("https://maven.aliyun.com/repository/public")
}

dependencies {
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.1.0")
}

tasks.named<Jar>("jar") {
    manifest {
        attributes["Name"] = "iTXTech MiraiNative"
        attributes["Revision"] = Runtime.getRuntime().exec("git rev-parse --short HEAD")
            .inputStream.bufferedReader().readText().trim()
    }
}

tasks.withType<org.jetbrains.kotlin.gradle.tasks.KotlinCompile> {
    kotlinOptions.jvmTarget = "1.8"
}
