fn main() {
    println!("cargo:rerun-if-changed=src/miniaudio/core.c");
    cc::Build::new()
        .file("src/miniaudio/core.c")
        .compile("miniaudio");
}