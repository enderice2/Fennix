use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_void;

unsafe extern "C" {
	fn malloc(size: usize) -> *mut c_void;
	fn free(ptr: *mut c_void);
}

struct FennixAllocator;

unsafe impl GlobalAlloc for FennixAllocator {
	unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
		unsafe { malloc(layout.size()) as *mut u8 }
	}

	unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
		unsafe {
			free(ptr as *mut c_void);
		}
	}
}

#[global_allocator]
static ALLOCATOR: FennixAllocator = FennixAllocator;
