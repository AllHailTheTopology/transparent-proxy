LINK = ip link
NETNS = ip netns
NETNS_EXEC = ip netns exec
NETNS_CLIENT = client
NETNS_PROXY = proxy
NETNS_SERVER = server

topology:
	# create network namespaces
	$(NETNS) add $(NETNS_CLIENT)
	$(NETNS) add $(NETNS_PROXY)
	$(NETNS) add $(NETNS_SERVER)

	# wire them together with veths
	$(LINK) add name client-a type veth peer name proxy-a
	$(LINK) set dev client-a netns $(NETNS_CLIENT)
	$(LINK) set dev proxy-a netns $(NETNS_PROXY)

	$(LINK) add name proxy-b type veth peer name server-a
	$(LINK) set dev proxy-b netns $(NETNS_PROXY)
	$(LINK) set dev server-a netns $(NETNS_SERVER)

	# configure IP networking
	$(NETNS_EXEC) $(NETNS_CLIENT) ip addr add 10.0.5.1/24 dev client-a
	$(NETNS_EXEC) $(NETNS_CLIENT) ip link set dev client-a up
	$(NETNS_EXEC) $(NETNS_CLIENT) ip route add default via 10.0.5.10

	$(NETNS_EXEC) $(NETNS_PROXY) ip addr add 10.0.5.10/24 dev proxy-a
	$(NETNS_EXEC) $(NETNS_PROXY) ip link set dev proxy-a up

	$(NETNS_EXEC) $(NETNS_PROXY) ip addr add 10.0.6.10/24 dev proxy-b
	$(NETNS_EXEC) $(NETNS_PROXY) ip link set dev proxy-b up

	$(NETNS_EXEC) $(NETNS_SERVER) ip addr add 10.0.6.1/24 dev server-a
	$(NETNS_EXEC) $(NETNS_SERVER) ip link set dev server-a up
	$(NETNS_EXEC) $(NETNS_SERVER) ip route add default via 10.0.6.10

	# enable arp proxy in proxy network namespace
	$(NETNS_EXEC) $(NETNS_PROXY) sysctl -w net.ipv4.ip_forward=1
	$(NETNS_EXEC) $(NETNS_PROXY) sysctl -w net.ipv4.conf.all.proxy_arp=1

topology-destroy:
	$(NETNS) del $(NETNS_CLIENT)
	$(NETNS) del $(NETNS_PROXY)
	$(NETNS) del $(NETNS_SERVER)

clean:
	rm -rf *.o
	rm -rf proxy
