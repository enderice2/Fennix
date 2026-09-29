/*
	This file is part of Fennix Kernel.

	Fennix Kernel is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	Fennix Kernel is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Fennix Kernel. If not, see <https://www.gnu.org/licenses/>.
*/

.code64

.global _sig_native_trampoline_start
_sig_native_trampoline_start:
	int $0x3

.global _sig_native_trampoline_end
_sig_native_trampoline_end:

.global _sig_linux_trampoline_start
_sig_linux_trampoline_start:
	movq %rsp, %rbp
	movq (%rbp), %rax
	call *%rax
	mov %rbp, %rsp
	/* rt_sigreturn = 15 */
	movq $15, %rax
	syscall

.global _sig_linux_trampoline_end
_sig_linux_trampoline_end:
