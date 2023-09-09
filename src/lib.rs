use std::ffi::c_float;
use std::os::raw::{c_uchar, c_void, c_int, c_uint};
use std::fs;

#[derive(Default, Debug, Copy, Clone)]
pub enum AudioUsage {
    Static,
    #[default]
    Stream
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct AudioParams {
    pub volume: c_float,
    pub pitch: c_float,
    pub loop_: c_uchar,
}

impl Default for AudioParams {
    fn default() -> Self {
        AudioParams {
            volume: 1.0,
            pitch: 0.0,
            loop_: 0,
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
struct AudioData {
    usage: c_int,
    size: c_uint,
    data: *mut c_void
}

impl Default for AudioData {
    fn default() -> Self {
        AudioData {
            usage: 0,
            size: 0,
            data: std::ptr::null_mut()
        }
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
struct Decoder([c_uchar; 10312]);
impl Default for Decoder {
    fn default() -> Self {
        Decoder([0; 10312])
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct AudioBuffer {
    id: u32,
    playing: i8,
    loaded: i8,
    offset: u32,
    params: AudioParams,
    data: AudioData,
    decoder: Decoder
}

impl ToString for AudioBuffer {
    fn to_string(&self) -> String {
        format!("{} {} {} {}", self.id, self.playing, self.loaded, self.data.size)
    }
}

impl Default for AudioBuffer {
    fn default() -> Self {
        AudioBuffer {
            id: 0,
            playing: 0,
            loaded: 0,
            offset: 0,
            params: AudioParams::default(),
            data: AudioData::default(),
            decoder: Decoder::default()
        }
    }
}

extern "C" {
    fn core_init() -> c_int;
    fn core_quit();

    // fn core_get_available_buffer() -> c_uint;
    // fn core_release_buffer(id: c_uint);

    fn core_new_data_from_memory(data: *mut c_void, size: c_uint, usage: c_int) -> c_int;
    fn core_release_data(id: c_int);
    fn core_setup_free_buffer(data_id: c_int, params: *const AudioParams) -> c_int;

    fn core_get_buffer(id: c_int) -> *mut AudioBuffer;
}

#[derive(Default, Debug, Copy, Clone)]
pub struct Audio {
    id: u32,
    params: AudioParams
}

impl Audio {
    pub fn play(&self) -> AudioInstance {
        let id = unsafe { core_setup_free_buffer(self.id as c_int, &self.params) } as u32;
        let buffer = get_buffer(id);
        buffer.playing = 1;
        AudioInstance(id)
    }

    pub fn free(&self) {
        unsafe { core_release_data(self.id as i32) };
    }

    pub fn id(&self) -> u32 { self.id }
    
    pub fn loop_(&self) -> bool {
        self.params.loop_ == 1
    }

    pub fn set_loop(&mut self, val: bool) {
        self.params.loop_ = val as u8;
    }
}

#[derive(Default, Debug, Copy, Clone)]
pub struct AudioInstance(u32);

pub fn init() { unsafe { core_init(); } }
pub fn quit() { unsafe { core_quit(); } }

pub fn load(filename: &str, usage: AudioUsage) -> Result<Audio, String> {
    let data = fs::read(filename)
        .expect(filename);
    let id = unsafe { core_new_data_from_memory(data.as_ptr() as *mut c_void, data.len() as u32, usage as i32) } as u32;

    Ok(Audio {
        id,
        params: AudioParams::default()
    })
}

fn get_buffer<'a>(id: u32) -> &'a mut AudioBuffer {
    unsafe { core_get_buffer(id as c_int).as_mut().unwrap() }
}

impl AudioInstance {
    fn get_buffer(&self) -> &mut AudioBuffer {
        get_buffer(self.0)
    }

    pub fn resume(&self) {
        let buffer = self.get_buffer();
        buffer.playing = 1;
    }

    pub fn pause(&self) {
        let buffer = self.get_buffer();
        buffer.playing = 0;
    }

    pub fn stop(&self) {
        let buffer = self.get_buffer();
        buffer.playing = 0;
        buffer.loaded = 0;
    }

    pub fn is_playing(&self) -> bool {
        let buffer = self.get_buffer();
        buffer.playing == 1
    }

    pub fn volume(&self) -> f32 { self.get_buffer().params.volume }
    pub fn set_volume(&self, volume: f32) {
        self.get_buffer().params.volume = volume;
    }

    pub fn looping(&self) -> bool { self.get_buffer().params.loop_ == 1 }
    pub fn set_looping(&self, val: bool) {
        self.get_buffer().params.loop_ = val as u8;
    }
}
