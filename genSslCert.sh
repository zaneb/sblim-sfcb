echo " Generating SSL certificates... "
CN="Common Name"
EMAIL="test@email.address"
HOSTNAME=`uname -n`
sed -e "s/$CN/$HOSTNAME/"  \
    -e "s/$EMAIL/root@$HOSTNAME/" ssl.orig \
    > ssl.cnf
chmod 644 ssl.cnf
chown bin ssl.cnf
chgrp bin ssl.cnf

openssl req -x509 -days 365 -newkey rsa:2048 \
   -nodes -config ssl.cnf   \
   -keyout key.pem -out cert.pem

cat key.pem > file_2048.pem
cat cert.pem > server_2048.pem
cat cert.pem > client_2048.pem
chmod 700 *.pem

rm -f key.pem cert.pem

if [ -f server.pem ]
then
    echo "WARNING: server.pem SSL Certificate file already exists."
else
    cp server_2048.pem server.pem
    cp file_2048.pem file.pem
    chmod 400 server.pem file.pem
fi

if [ -f client.pem ]
then
    echo "WARNING: client.pem SSL Certificate trust store already exists."
else
    cp client_2048.pem client.pem
    chmod 400 client.pem
fi
