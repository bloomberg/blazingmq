#!/bin/bash
#
#
# This scripts generates:
#  - root CA certificate
#  - server certificate and keystore
#  - client keys
#
# Based off of
#   https://github.com/confluentinc/librdkafka/blob/master/tests/gen-ssl-certs.sh

OP="$1"
CA_CERT="$2"
PFX="$3"
HOST="$4"

C=NN
ST=NN
L=NN
O=NN
OU=NN
CN="$HOST"
 

# Password
PASS="secret"

# Cert validity, in days
VALIDITY=10000

set -e

export LC_ALL=C

if [[ $OP == "ca" && -n "$CA_CERT" && -n "$3" ]]; then
    CN="$3"
    openssl req -new -x509 -keyout "${CA_CERT}.key" -out "${CA_CERT}" -days $VALIDITY -passin "pass:$PASS" -passout "pass:$PASS" <<EOF
${C}
${ST}
${L}
${O}
${OU}
${CN}
$USER@${CN}
.
.
EOF



elif [[ $OP == "server" && -n "$CA_CERT" && -n "$PFX" && -n "$CN" ]]; then
    HOST_CERT_CONFIG_PATH="${PFX}host_cert.cnf"
    HOST_PRIVATE_RSA_KEY_PATH="${PFX}host_private_key_rsa.pem"
    HOST_PRIVATE_KEY_PATH="${PFX}private_key.pem"
    HOST_CSR_PATH="${PFX}host_csr.pem"
    HOST_CERT_PATH="${PFX}host_cert.pem"
    HOST_CERT_CHAIN_PATH="${PFX}client_${CN}.pem"

    # Create the CA cert config file
    echo "Setting up host certs..."

    cat <<EOF > "${HOST_CERT_CONFIG_PATH}"
[req]
default_bits        = 2048
distinguished_name  = req_distinguished_name
req_extensions      = v3_req
prompt              = no
[req_distinguished_name]
C   = ${C}
ST  = ${ST}
L   = ${L}
O   = ${O}
CN  = ${CN}
[v3_req]
subjectAltName = @alt_names
[alt_names]
DNS.1  = ${CN}
DNS.2  = localhost.
EOF

    #Step 1
    echo "############ Generating key"
    openssl genrsa -out "${HOST_PRIVATE_RSA_KEY_PATH}" 2048
    openssl pkcs8 -nocrypt -topk8 -v1 PBE-SHA1-RC4-128 -inform pem -outform pem -in "${HOST_PRIVATE_RSA_KEY_PATH}" -out "${HOST_PRIVATE_KEY_PATH}"
	
    #Step 2
    echo "############ Generate the CSR"
    openssl req -nodes -new -extensions v3_req -sha256 -config "${HOST_CERT_CONFIG_PATH}" -key "${HOST_PRIVATE_KEY_PATH}" -out "${HOST_CSR_PATH}"
    
    #Step 3
    echo "############ Generate the cert"
    openssl x509 -req -in "${HOST_CSR_PATH}" -CA "${CA_CERT}" -CAkey "${CA_CERT}.key" -CAcreateserial -out "${HOST_CERT_PATH}" -days ${VALIDITY} -sha256 -extensions v3_req -extfile "${HOST_CERT_CONFIG_PATH}" -passin "pass:${PASS}"
    
    cat "${HOST_CERT_PATH}" > "${HOST_CERT_CHAIN_PATH}"


elif [[ $OP == "client" && -n "$CA_CERT" && -n "$PFX" && -n "$CN" ]]; then

# Standard OpenSSL keys
	echo "############ Generating key"
	openssl genrsa -nodes -passout "pass:${PASS}" -out "${PFX}client.key" 2048 
	
	echo "############ Generating request"
	openssl req -passin "pass:${PASS}" -passout "pass:${PASS}" -key "${PFX}client.key" -new -out "${PFX}client.req" \
		<<EOF
$C
$ST
$L
$O
$OU
$CN
.
$PASS
.
EOF

	echo "########### Signing key"
	openssl x509 -req -passin "pass:${PASS}" -in "${PFX}client.req" -CA "${CA_CERT}" -CAkey "${CA_CERT}.key" -CAcreateserial -out "${PFX}client.pem" -days ${VALIDITY}


else
    echo "Usage: $0 ca <ca-cert-file> <CN>"
    echo "       $0 server|client <ca-cert-file> <file_prefix> <hostname>"
    echo ""
    exit 1
fi

