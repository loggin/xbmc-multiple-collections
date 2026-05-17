echo -ne "\033]0;install base system\007"
pacman --noconfirm -S $(cat /etc/pac-base-new.pk | sed -e 's#\\##')
sleep 3
exit
