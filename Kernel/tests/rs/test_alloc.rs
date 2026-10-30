use crate::alloc::borrow::ToOwned;
use alloc::{
	alloc::{alloc, alloc_zeroed, dealloc, realloc},
	boxed::Box,
	collections::{BTreeMap, BTreeSet, BinaryHeap, LinkedList, VecDeque},
	rc::Rc,
	string::{String, ToString},
	sync::Arc,
	vec,
	vec::Vec,
};
use core::alloc::Layout;
use core::cell::{Cell, RefCell};
use core::num::NonZeroUsize;
use core::ptr::NonNull;

fn vec_test() {
	let mut vector = Vec::new();

	vector.push(10u64);
	vector.push(20);
	vector.push(30);

	assert_eq!(vector, [10, 20, 30]);

	vector.reserve(16);
	assert!(vector.capacity() >= 19);

	vector.insert(1, 15);
	assert_eq!(vector, [10, 15, 20, 30]);

	assert_eq!(vector.remove(1), 15);

	vector.extend([40, 50]);
	assert_eq!(vector, [10, 20, 30, 40, 50]);

	vector.retain(|x| *x >= 20);
	assert_eq!(vector, [20, 30, 40, 50]);

	vector.sort();
	vector.dedup();

	assert_eq!(vector.pop(), Some(50));

	vector.clear();
	assert!(vector.is_empty());

	let mut vector = vec![1, 2, 3, 4];

	vector.resize(8, 5);
	assert_eq!(vector, [1, 2, 3, 4, 5, 5, 5, 5]);

	vector.truncate(6);
	assert_eq!(vector, [1, 2, 3, 4, 5, 5]);

	let split = vector.split_off(2);
	assert_eq!(vector, [1, 2]);
	assert_eq!(split, [3, 4, 5, 5]);

	let cloned = vector.clone();
	assert_eq!(cloned, vector);

	let boxed = vector.into_boxed_slice();
	assert_eq!(&*boxed, &[1, 2]);
}

fn string_test() {
	let mut string = String::new();

	string.push_str("Fennix");
	string.push(' ');
	string.push_str("Rust");

	assert_eq!(string, "Fennix Rust");

	string.reserve(32);

	string.insert_str(0, "Kernel ");
	assert_eq!(string, "Kernel Fennix Rust");

	string.replace_range(0..7, "Fennix ");
	assert_eq!(string, "Fennix Fennix Rust");

	let cloned = string.clone();
	assert_eq!(cloned, string);

	let converted = string.to_string();
	assert_eq!(converted, string);

	let owned = string.to_owned();
	assert_eq!(owned, string);

	let bytes = string.into_bytes();
	assert_eq!(bytes, b"Fennix Fennix Rust");
}

fn box_test() {
	let boxed = Box::new(1234u64);

	assert_eq!(*boxed, 1234);

	let boxed_slice = vec![1u8, 2, 3, 4].into_boxed_slice();

	assert_eq!(boxed_slice.len(), 4);
	assert_eq!(&*boxed_slice, &[1, 2, 3, 4]);
}

fn rc_test() {
	let rc = Rc::new(String::from("Fennix Rc"));

	assert_eq!(Rc::strong_count(&rc), 1);

	let rc_clone = Rc::clone(&rc);

	assert_eq!(Rc::strong_count(&rc), 2);
	assert_eq!(&*rc_clone, "Fennix Rc");

	drop(rc_clone);

	assert_eq!(Rc::strong_count(&rc), 1);

	let value = Rc::try_unwrap(rc).expect("Rc::try_unwrap failed");
	assert_eq!(value, "Fennix Rc");
}

fn arc_test() {
	let arc = Arc::new(String::from("Fennix Arc"));

	assert_eq!(Arc::strong_count(&arc), 1);

	let arc_clone = Arc::clone(&arc);

	assert_eq!(Arc::strong_count(&arc), 2);
	assert_eq!(&*arc_clone, "Fennix Arc");

	drop(arc_clone);

	assert_eq!(Arc::strong_count(&arc), 1);

	let value = Arc::try_unwrap(arc).expect("Arc::try_unwrap failed");
	assert_eq!(value, "Fennix Arc");
}

fn btreemap_test() {
	let mut map = BTreeMap::new();

	assert_eq!(map.insert(1, String::from("one")), None);
	assert_eq!(map.insert(2, String::from("two")), None);

	assert_eq!(map.get(&1).map(|s| s.as_str()), Some("one"));

	*map.get_mut(&2).unwrap() = String::from("TWO");
	assert_eq!(map.get(&2).map(|s| s.as_str()), Some("TWO"));

	assert_eq!(map.remove(&1), Some(String::from("one")));

	let entry = map.entry(3).or_insert(String::from("three"));
	assert_eq!(entry, "three");

	assert_eq!(map.len(), 2);
	assert_eq!(map.iter().count(), 2);

	map.clear();
	assert!(map.is_empty());
}

fn btreeset_test() {
	let mut set = BTreeSet::new();

	assert!(set.insert(10));
	assert!(set.insert(20));
	assert!(set.insert(30));
	assert!(!set.insert(10));

	assert!(set.contains(&10));
	assert!(set.remove(&20));
	assert!(!set.contains(&20));

	let other = BTreeSet::from([30, 40]);

	assert_eq!(set.union(&other).copied().collect::<Vec<_>>(), [10, 30, 40]);

	set.clear();
	assert!(set.is_empty());
}

fn binaryheap_test() {
	let mut heap = BinaryHeap::new();

	heap.push(10);
	heap.push(30);
	heap.push(20);

	assert_eq!(heap.len(), 3);
	assert_eq!(heap.peek(), Some(&30));
	assert_eq!(heap.pop(), Some(30));
	assert_eq!(heap.pop(), Some(20));
	assert_eq!(heap.pop(), Some(10));
	assert_eq!(heap.pop(), None);

	heap.clear();
	assert!(heap.is_empty());
}

fn vecdeque_test() {
	let mut deque = VecDeque::new();

	deque.push_back(10);
	deque.push_front(20);

	assert_eq!(deque.front(), Some(&20));
	assert_eq!(deque.back(), Some(&10));
	assert_eq!(deque.len(), 2);

	assert_eq!(deque.pop_back(), Some(10));
	assert_eq!(deque.pop_front(), Some(20));

	assert!(deque.is_empty());

	deque.reserve(16);
	assert!(deque.capacity() >= 16);

	deque.clear();
}

fn linkedlist_test() {
	let mut list = LinkedList::new();

	list.push_back(10);
	list.push_front(20);

	assert_eq!(list.front(), Some(&20));
	assert_eq!(list.back(), Some(&10));
	assert_eq!(list.len(), 2);

	assert_eq!(list.pop_back(), Some(10));
	assert_eq!(list.pop_front(), Some(20));

	assert!(list.is_empty());

	list.clear();
}

fn cell_test() {
	let cell = Cell::new(10);

	assert_eq!(cell.get(), 10);

	cell.set(20);

	assert_eq!(cell.get(), 20);
}

fn refcell_test() {
	let refcell = RefCell::new(String::from("Fennix"));

	assert_eq!(refcell.borrow().as_str(), "Fennix");

	refcell.borrow_mut().push_str(" OS");

	assert_eq!(refcell.borrow().as_str(), "Fennix OS");
}

fn raw_alloc_test() {
	unsafe {
		let layout = Layout::from_size_align(64, 8).unwrap();

		let ptr = alloc(layout);

		assert!(!ptr.is_null(), "alloc returned null");

		ptr.write_bytes(0xAA, 64);

		for i in 0..64 {
			assert_eq!(ptr.add(i).read(), 0xAA);
		}

		dealloc(ptr, layout);

		let zeroed = alloc_zeroed(layout);

		assert!(!zeroed.is_null(), "alloc_zeroed returned null");

		for i in 0..64 {
			assert_eq!(zeroed.add(i).read(), 0);
		}

		dealloc(zeroed, layout);

		let old_layout = Layout::from_size_align(32, 8).unwrap();
		let new_size = NonZeroUsize::new(128).unwrap();
		let new_layout = Layout::from_size_align(new_size.get(), 8).unwrap();

		let ptr = alloc(old_layout);

		assert!(!ptr.is_null(), "alloc for realloc returned null");

		ptr.write_bytes(0xBB, 32);

		let new_ptr = realloc(ptr, old_layout, new_size.get());

		if new_ptr.is_null() {
			dealloc(ptr, old_layout);
			panic!("realloc returned null");
		}

		for i in 0..32 {
			assert_eq!(new_ptr.add(i).read(), 0xBB);
		}

		dealloc(new_ptr, new_layout);
	}
}

fn nonnull_test() {
	let value = 42u64;
	let ptr = NonNull::from(&value);

	assert_eq!(ptr, NonNull::from(&value));

	let reference = unsafe { ptr.as_ref() };

	assert_eq!(*reference, 42);
}

pub fn rust_alloc_test() {
	vec_test();
	string_test();
	box_test();
	rc_test();
	arc_test();
	btreemap_test();
	btreeset_test();
	binaryheap_test();
	vecdeque_test();
	linkedlist_test();
	cell_test();
	refcell_test();
	raw_alloc_test();
	nonnull_test();
}
