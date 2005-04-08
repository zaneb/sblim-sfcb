install: 
	make -C sblim-indication_helpers -f makefile install
	make -C sblim-cmpi-base install
	rm schema/Linux_BaseRegistration.mof
	mv schema/Linux_Base.mof schema/Linux_Base.cimv2mof
	mkdir -p  ../repository/root/cimv2      
	sfcbmof -I $(CIMDIR) -i CIM_Schema.mof -o ../repository/root/cimv2/classSchemas schema/Linux_Base.cimv2mof
	make -C sblim-wbemcli install
	HOME=`pwd`/.. make -C sblim-cmpi-base/test install
