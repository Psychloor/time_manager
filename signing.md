- Windows:
    - Configure with signing: -D TIME_MANAGER_BUILD_SHARED=ON -D TIME_MANAGER_SIGN_WINDOWS=ON -D TIME_MANAGER_SIGN_WINDOWS_CERT="C:/path/to/cert.pfx" -D TIME_MANAGER_SIGN_WINDOWS_CERT_PASSWORD="secret" -D TIME_MANAGER_COMPANY_NAME="Your Company" -D TIME_MANAGER_COPYRIGHT="(C) 2025 Your Company"
    - Result: DLL will have VERSIONINFO and be signed post-build if signtool is found.

- macOS:
    - Configure: -D TIME_MANAGER_BUILD_SHARED=ON -D TIME_MANAGER_CODESIGN_IDENTITY="Developer ID Application: Your Company (TEAMID)"
    - Result: dylib will be codesigned post-build.

- Linux:
    - Configure to embed metadata: -D TIME_MANAGER_BUILD_SHARED=ON -D TIME_MANAGER_EMBED_ELF_METADATA=ON
    - Result: .so will contain a .note.time_manager section with JSON metadata and a build-id (if supported).

Optional: detached signatures for Linux
- Create a detached signature after build (to distribute alongside libtime_manager.so):
``` bash
# Using GPG
gpg --output libtime_manager.so.sig --detach-sign --armor libtime_manager.so

# Or using OpenSSL CMS (requires a signing cert and key)
openssl cms -sign -binary -in libtime_manager.so -signer cert.pem -inkey key.pem -outform DER -out libtime_manager.so.p7s -nodetach -nosmimecap -noattr
```
Verification:
- Windows: signtool verify /pa /v path\to\time_manager.dll
- macOS: codesign --verify --deep --verbose path/to/libtime_manager.dylib
- Linux:
    - Read metadata: readelf -n libtime_manager.so | less, or objdump -s -j .note.time_manager libtime_manager.so
    - Verify detached signature per your chosen method.
