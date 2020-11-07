# b2-uploader
Simple C binary to upload objects to BackBlaze's B2 buckets
Useful for simple programs which need to upload single files to B2.

# BUILDING
## NOTES
SecretString.h contains all private _user specific_ build information such as api keys and bucket name.
See `make Obfuscator` below to generate the APIKey string for use in b2UploadSec1 variable.

## Targets
`make` with no arguments will generate a Dev build which prints A LOT of debug info, including function names and code lines.
`make release` will generate a build which only prints errors, and does not have useful development debug information like function names or line numbers.

`make Obfuscator` generates the Obfuscator binary which is used to _securely_ encode your private key information into a "secret" string.
