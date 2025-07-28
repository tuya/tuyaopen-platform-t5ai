set(srcs)
set(incs)
set(priv_incs)

function(zephyr_library_sources_ifdef feature_toggle path source)
  if(${${feature_toggle}})
    set(srcs ${srcs} ${path}/${source} PARENT_SCOPE)
  endif()
endfunction()

function(zephyr_library_sources_ifndef feature_toggle path source)
  if(${${feature_toggle}})
  else()
    set(srcs ${srcs} ${path}/${source} PARENT_SCOPE)
  endif()
endfunction()

if (CONFIG_BLE_MESH_ZEPHYR)

    list(APPEND incs
        ../../../include/components/bluetooth/
        ../../../include/components/
        ../../../include/
        include/
        subsys/bluetooth/
        sample
    )

    list(APPEND srcs
		subsys/bluetooth/common/log.c
		subsys/bluetooth/common/rpa.c
    )

    list(APPEND srcs
        subsys/net/net_buf.c
    )

    list(APPEND srcs
        subsys/sys/atomic_c.c
        subsys/sys/z_mem_slab.c
        subsys/sys/z_poll.c
        subsys/sys/z_queue.c
        subsys/sys/z_work.c
        subsys/sys/zephy_rtos.c
        subsys/sys/zephyr_work_thread.c
    )

    list(APPEND srcs
        subsys/tinycrypt/src/ble_aes_decrypt.c
		subsys/tinycrypt/src/ble_aes_encrypt.c
		subsys/tinycrypt/src/cbc_mode.c
		subsys/tinycrypt/src/ccm_mode.c
		subsys/tinycrypt/src/cmac_mode.c
		subsys/tinycrypt/src/ctr_mode.c
		subsys/tinycrypt/src/ctr_prng.c
		subsys/tinycrypt/src/ecc_dh.c
		subsys/tinycrypt/src/ecc_dsa.c
		subsys/tinycrypt/src/ecc_platform_specific.c
		subsys/tinycrypt/src/hmac_prng.c
		subsys/tinycrypt/src/ty_ecc.c
		subsys/tinycrypt/src/ty_hmac.c
		subsys/tinycrypt/src/ty_sha256.c
		subsys/tinycrypt/src/ty_utils.c

    )

    list(APPEND srcs
        subsys/bluetooth/host/uuid.c
        subsys/bluetooth/host/addr.c
        subsys/bluetooth/host/buf.c
        subsys/bluetooth/host/hci_core.c
        subsys/bluetooth/host/hci_common.c
        subsys/bluetooth/host/id.c
        subsys/bluetooth/host/adv.c
        subsys/bluetooth/host/scan.c
        subsys/bluetooth/host/ethermind_impl.c
        subsys/bluetooth/host/crypto.c
        subsys/bluetooth/host/ecc.c
        subsys/bluetooth/host/hci_driver.c
    )

    list(APPEND srcs
        subsys/bluetooth/mesh/main.c
        subsys/bluetooth/mesh/cfg.c
        subsys/bluetooth/mesh/adv.c
        subsys/bluetooth/mesh/beacon.c
        subsys/bluetooth/mesh/net.c
        subsys/bluetooth/mesh/subnet.c
        subsys/bluetooth/mesh/app_keys.c
        subsys/bluetooth/mesh/transport.c
        subsys/bluetooth/mesh/rpl.c
        subsys/bluetooth/mesh/heartbeat.c
        subsys/bluetooth/mesh/crypto.c
        subsys/bluetooth/mesh/access.c
        subsys/bluetooth/mesh/msg.c
        subsys/bluetooth/mesh/cfg_srv.c
        subsys/bluetooth/mesh/health_srv.c
    )

    list(APPEND srcs
			sample/ble_mesh_ui.c
			sample/ble_mesh_tmall_spirit_sample.c
			sample/ble_mesh_led_sample.c
			sample/ble_mesh_provision_sample.c
    )

    zephyr_library_sources_ifdef(CONFIG_BT_MESH_ADV_LEGACY      subsys/bluetooth/mesh adv_legacy.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_ADV_EXT         subsys/bluetooth/mesh adv_ext.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_SETTINGS        subsys/bluetooth/mesh settings.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_LOW_POWER       subsys/bluetooth/mesh lpn.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_FRIEND          subsys/bluetooth/mesh friend.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROV            subsys/bluetooth/mesh prov.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROV_DEVICE     subsys/bluetooth/mesh prov_device.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_PROVISIONER     subsys/bluetooth/mesh provisioner.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_ADV          subsys/bluetooth/mesh pb_adv.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_PB_GATT         subsys/bluetooth/mesh pb_gatt.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_GATT_SERVER     subsys/bluetooth/mesh gatt_services.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_GATT            subsys/bluetooth/mesh proxy_msg.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_CFG_CLI         subsys/bluetooth/mesh cfg_cli.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_HEALTH_CLI      subsys/bluetooth/mesh health_cli.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_SELF_TEST       subsys/bluetooth/mesh test.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_SHELL           subsys/bluetooth/mesh shell.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_SHELL           subsys/bluetooth/shell bt.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_CDB             subsys/bluetooth/mesh cdb.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_CONN            subsys/bluetooth/host conn.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_CONN            subsys/bluetooth/host l2cap.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_CONN            subsys/bluetooth/host att.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_CONN            subsys/bluetooth/host gatt.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_SMP             subsys/bluetooth/host smp.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_SMP             subsys/bluetooth/host keys.c)
    zephyr_library_sources_ifndef(CONFIG_BT_MESH_SMP            subsys/bluetooth/host smp_null.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_ISO             subsys/bluetooth/host iso.c)
    zephyr_library_sources_ifdef(CONFIG_BT_MESH_HOST_CCM        subsys/bluetooth/host aes_ccm.c)



	include(${CMAKE_CURRENT_LIST_DIR}/mesh.cmake)
    set(definitions ${MESH_MACRO_CONFIG})
	get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
	set(config_component_name ${config_component_name})

    armino_component_register_ex(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_incs}" CONFIG_COMPONENT_NAME "${config_component_name}")
    armino_component_get_target(target "${config_component_name}")

    target_compile_definitions(${target} PRIVATE ${definitions})
else()
    armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_incs}")
endif()
