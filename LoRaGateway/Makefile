### Environment constants 

ARCH ?=
CROSS_COMPILE ?=
export

### general build targets

all:
	$(MAKE) all -e -C libloragw
	$(MAKE) all -e -C libstack_ipv6
	$(MAKE) all -e -C util_pkt_logger
	$(MAKE) all -e -C util_spi_stress
	$(MAKE) all -e -C util_lbt_test
	$(MAKE) all -e -C util_tx_test
	$(MAKE) all -e -C util_tx_continuous
	$(MAKE) all -e -C util_spectral_scan
	$(MAKE) all -e -C project_ipv6

clean:
	$(MAKE) clean -e -C libloragw
	$(MAKE) clean -e -C libstack_ipv6
	$(MAKE) clean -e -C util_pkt_logger
	$(MAKE) clean -e -C util_spi_stress
	$(MAKE) clean -e -C util_lbt_test
	$(MAKE) clean -e -C util_tx_test
	$(MAKE) clean -e -C util_tx_continuous
	$(MAKE) clean -e -C util_spectral_scan
	$(MAKE) clean -e -C project_ipv6

### EOF
