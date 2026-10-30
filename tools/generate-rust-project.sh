#!/usr/bin/env bash
set -euo pipefail

SYSROOT="$(rustc --print sysroot)"
KERNEL_DIR="$(cd "$(dirname "$0")/../Kernel" && pwd)"

cat > "$KERNEL_DIR/rust-project.json" <<EOF
{
	"sysroot": "$SYSROOT",
	"sysroot_src": "$SYSROOT/lib/rustlib/src/rust/library",
	"crates": [
		{
			"root_module": "kernel.rs",
			"edition": "2024",
			"deps": [],
			"cfg": [
				"target_os=\\"none\\"",
				"target_arch=\\"x86_64\\""
			],
			"env": {},
			"is_workspace_member": true
		}
	]
}
EOF

echo "Generated rust-project.json using: $SYSROOT"
