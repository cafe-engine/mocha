# Mocha

Mocha is a lib for handle audio using miniaudio, my plan is to exclude miniaudio dependency for use Rust only to make an audio engine.

The project is one of the modules of the [`cafe`](https://crates.io/crates/cafe_core) project.

```rust
extern crate mocha;

use mocha::{Audio, AudioUsage};

fn main() {
    mocha::init();
    let audio = mocha::load("audio.wav", AudioUsage::Stream).unwrap();
    let instance = audio.play();
    while instance.is_playing() {}
    mocha::quit();
}
```

