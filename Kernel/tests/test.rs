#[path = "rs/test_alloc.rs"]
pub mod test_alloc;

#[unsafe(no_mangle)]
pub extern "C" fn rust_run_tests() -> () {
	let result = test_alloc::rust_alloc_test();
	let _ = result;
}
