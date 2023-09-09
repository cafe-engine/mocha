extern crate mocha;

use mocha::{Audio, AudioUsage};

fn main() {
    mocha::init();
    let audio = mocha::load("audio.wav", AudioUsage::Stream).unwrap();
    let instance = audio.play();
    while instance.is_playing() {}
    mocha::quit();
}
