echo "  Removing leftover indication objects"
rm -f $SFCB_HOME/repository/root/interop/cim_indicationfilter*
rm -f $SFCB_HOME/repository/root/interop/cim_listenerdestinationcimxml*
rm -f $SFCB_HOME/repository/root/interop/cim_indicationsubscription*
rm -f $SFCB_HOME/indication.log
echo "  Restarting sfcBroker"
killall -1 sfcBroker
sleep 3
echo "  Starting indication tests"
catdiff CreateFilter01
catdiff CreateHandler01
catdiff CreateSubscription01
echo "  Waiting 10 seconds"
sleep 10
catdiff DeleteSubscription01
catdiff DeleteHandler01
catdiff DeleteFilter01
