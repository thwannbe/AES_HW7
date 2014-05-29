#sudo ./mount.sh
while true
do
sudo bonnie++ -s 100:1024 -n 20 -r 10 -x 1 -b -z 1 -u root -d /media/blueSSD
done
#sudo ./umount.sh
