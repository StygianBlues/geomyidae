#!/bin/sh

keyfile="geomyidae.key"
csrfile="geomyidae.csr"
certfile="geomyidae.crt"

# Generate the private key.
openssl genrsa -out "${keyfile}" 4096
# Generate signing request.
openssl req -new -key "${keyfile}" -out "${csrfile}"
# Sign the request ourself.
openssl x509 -sha256 -req -days 365 -in "${csrfile}" \
	-signkey "${keyfile}" -out "${certfile}"

