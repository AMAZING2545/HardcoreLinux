# ISO verification

An Obsidian Core Linux ISO is a **verified test artifact** only when the qualification workflow for the exact commit has passed all required gates.

## Qualification gates

1. Build and verify the distribution kernel, modules, and firmware.
2. Build the live root filesystem.
3. Create the GRUB/El Torito ISO.
4. Verify the ISO payload, UEFI loader, embedded EFI filesystem, and initramfs.
5. Boot the live ISO under UEFI QEMU.
6. Boot the live ISO under legacy BIOS QEMU.
7. Run the real installer against a disposable virtual disk.
8. Verify the installed root and EFI payload.
9. Boot the installed system under UEFI QEMU.
10. Generate a SHA-256 digest for the exact ISO.

## Hardware scope

A passing VM qualification proves bootability on the tested x86_64 QEMU configurations. It does **not** prove compatibility with every physical PC, GPU, Wi-Fi chipset, audio codec, suspend state, or GNOME component.

The current hardware target is x86_64 UEFI. First physical testing should use the live ISO from a dedicated USB drive while keeping existing disks untouched.

The installer performs destructive disk operations only after explicit disk selection and an exact confirmation such as `ERASE /dev/sda`.

## Secure Boot

The current ISO uses a project-generated UEFI GRUB loader and is not enrolled in a Microsoft/firmware Secure Boot trust chain. Disable Secure Boot before physical testing unless the firmware is configured to trust your own signing key.

## Integrity

Always verify the published SHA-256 digest before writing an ISO to USB. SHA-256 verifies the downloaded bytes; it does not by itself prove developer identity.

For a GitHub-published release, also verify the build provenance attestation:

```bash
gh attestation verify ObsidianCoreLinux-v0.3.0-rc.1.iso \
  -R Yassine-Jemi01/HardcoreLinux
```

The attestation links the ISO to the repository and workflow that produced it. It is provenance evidence, not a claim that every component of the operating system is secure. GitHub documents artifact attestations as a way to establish software build provenance.

## Current installation layout

The installer currently targets an EFI System Partition plus an ext4 root filesystem on x86_64 systems.
