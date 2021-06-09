plugins {
    kotlin("jvm") version "1.4.32"
    kotlin("plugin.serialization") version "1.4.32"
    `maven-publish`

    id("net.mamoe.mirai-console") version "2.6.5"
}

group = "org.itxtech"
version = "2.0.0-beta.1"
description = "强大的 mirai 原生插件加载器。"
val vcs = "https://github.com/iTXTech/mirai-native"

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
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.0.1")
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

@Suppress("DEPRECATION")
val sourcesJar by tasks.registering(Jar::class) {
    classifier = "sources"
    from(sourceSets["main"].allSource)
}

publishing {
    publications {
        register("mavenJava", MavenPublication::class) {
            from(components["java"])

            groupId = rootProject.group.toString()
            this.artifactId = artifactId
            version = version

            pom.withXml {
                val root = asNode()
                root.appendNode("description", description)
                root.appendNode("name", project.name)
                root.appendNode("url", vcs)
                root.children().last()
            }
            artifact(sourcesJar.get())
        }
    }
}
