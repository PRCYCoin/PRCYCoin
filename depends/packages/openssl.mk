package=openssl
$(package)_version=1.1.1w
$(package)_download_path=https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_1w
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8

define $(package)_set_vars
$(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)"
$(package)_config_opts=--prefix=$(host_prefix) --openssldir=$(host_prefix)/etc/openssl
# Conservative no-* set: this mirrors what the OLD, working 1.0.2u recipe disabled
# (translated to 1.1.1), NOT the aggressive slim used by forks on Qt 5.15 / curl 7.78.
# PRCY's bundled Qt 5.12.11 and curl 8.5.0 reference several OpenSSL features
# unconditionally, so disabling them breaks the cross-build here:
#   no-psk -> Qt qsslsocket_openssl.cpp: 'q_ssl_psk_use_session_callback' not declared
#   no-md4 -> curl md4.c falls back to the Windows CryptoAPI, fails under mingw
#   (no-dtls / no-srp / no-rc4 similarly risk Qt/curl references)
# Only genuinely-unused / insecure bits are disabled; everything else stays enabled.
#
# no-engine is the exception that MUST be disabled for the mingw cross-build: with the
# ENGINE framework on, OpenSSL 1.1.1's built-in Windows CAPI engine pulls in -lcrypt32,
# which curl's OpenSSL-detection link probe (HMAC_Update in -lcrypto) does not link, so
# curl's configure aborts with "--with-openssl was given but OpenSSL could not be
# detected". The wallet uses no OpenSSL engines and Qt/curl guard engine code behind
# OPENSSL_NO_ENGINE, so this is safe.
$(package)_config_opts+=no-shared
$(package)_config_opts+=no-tests
$(package)_config_opts+=no-comp
$(package)_config_opts+=no-dso
$(package)_config_opts+=no-engine
$(package)_config_opts+=no-dynamic-engine
$(package)_config_opts+=no-ssl3
$(package)_config_opts+=no-ssl-trace
$(package)_config_opts+=no-weak-ssl-ciphers
$(package)_config_opts+=no-md2
$(package)_config_opts+=no-rc5
$(package)_config_opts+=no-rfc3779
$(package)_config_opts+=no-sctp
$(package)_config_opts+=no-zlib
$(package)_config_opts+=no-zlib-dynamic
$(package)_config_opts+=$($(package)_cflags) $($(package)_cppflags)
$(package)_config_opts_linux=-fPIC -Wa,--noexecstack
$(package)_config_opts_x86_64_linux=linux-x86_64
$(package)_config_opts_i686_linux=linux-generic32
$(package)_config_opts_arm_linux=linux-generic32
$(package)_config_opts_armv7l_linux=linux-generic32
$(package)_config_opts_aarch64_linux=linux-generic64
$(package)_config_opts_mipsel_linux=linux-generic32
$(package)_config_opts_mips_linux=linux-generic32
$(package)_config_opts_powerpc_linux=linux-generic32
$(package)_config_opts_riscv32_linux=linux-generic32
$(package)_config_opts_riscv64_linux=linux-generic64
$(package)_config_opts_powerpc64le_linux=linux-generic64
$(package)_config_opts_sparc64_linux=linux-generic64
$(package)_config_opts_s390x_linux=linux-generic64
$(package)_config_opts_alpha_linux=linux-generic64
$(package)_config_opts_m68k_linux=linux-generic32
$(package)_config_opts_x86_64_darwin=darwin64-x86_64-cc
$(package)_config_opts_x86_64_mingw32=mingw64
$(package)_config_opts_i686_mingw32=mingw
$(package)_config_opts_android=-fPIC
$(package)_config_opts_aarch64_android=linux-generic64
$(package)_config_opts_x86_64_android=linux-generic64
$(package)_config_opts_armv7a_android=linux-generic32
$(package)_config_opts_i686_android=linux-generic32
endef

# OpenSSL 1.1.x uses a different build system from 1.0.x (there is no Makefile.org,
# and the reproducible-build date lives in util/mkbuildinf.pl). These seds are
# best-effort for reproducibility; if the exact strings ever move they simply become
# no-ops (build_libs never builds apps/test anyway), so they cannot break the build.
define $(package)_preprocess_cmds
  sed -i.old 's/built on: $$date/built on: not available/' util/mkbuildinf.pl && \
  sed -i.old "s|\"engines\", \"apps\", \"test\"|\"engines\"|" Configure
endef

define $(package)_config_cmds
  ./Configure $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) -j1 build_libs libcrypto.pc libssl.pc openssl.pc
endef

# 1.1.x renamed the install-prefix variable from INSTALL_PREFIX (1.0.x) to DESTDIR.
define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) -j1 install_sw
endef

define $(package)_postprocess_cmds
  rm -rf share bin etc
endef
