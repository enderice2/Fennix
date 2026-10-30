#![no_std]

use core::panic::PanicInfo;

#[path = "library/rust/lib.rs"]
mod rust_library;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}
