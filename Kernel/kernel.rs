#![no_std]

extern crate alloc;

use core::panic::PanicInfo;

#[path = "library/rust/lib.rs"]
mod rust_library;

#[path = "tests/test.rs"]
mod rust_test;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
	loop {
		core::hint::spin_loop();
	}
}
