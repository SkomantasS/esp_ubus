include $(TOPDIR)/rules.mk

PKG_NAME:=esp_ubus
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/esp_ubus
	CATEGORY:=Tuya
	TITLE:=esp_ubus
	DEPENDS:= +libubus +libubox +libserialport +libblobmsg-json
endef
# +tuya-SDK-libs
define Package/esp_ubus/description
	This is a package that communicates with tuya cloud
endef

define Package/esp_ubus/install
	$(INSTALL_DIR) $(1)/usr/bin
#	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/esp_ubus $(1)/usr/bin
	$(INSTALL_BIN) ./files/esp_ubus.init $(1)/etc/init.d/esp_ubus
#	$(INSTALL_CONF) ./files/esp_ubus.config $(1)/etc/config/esp_ubus
endef

$(eval $(call BuildPackage,esp_ubus))
