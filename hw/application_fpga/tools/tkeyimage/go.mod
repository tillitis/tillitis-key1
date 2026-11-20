module tkeyimage

go 1.23.1

replace github.com/tillitis/tkey-sign-cli => ../../../../../tkey-sign-cli/

require (
	github.com/tillitis/tkey-sign-cli v1.1.1
	golang.org/x/crypto v0.40.0
)

require golang.org/x/sys v0.34.0 // indirect
