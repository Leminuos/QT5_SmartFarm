## Change line gpio

Commit này sẽ fix issue không thể điều khiển gpio, dù đã request gpio thành công.

Quá trình tìm kiếm:
- thử khi sử dụng viết một driver để điều khiển gpio -> fail.
- sử dụng `gpioset --mode=wait gpiochip1 2=1` -> fail
- quyết định viết một device tree với pinmuxing gpio -> điểm đột phá

Viết device tree như sau:


```
&am33xx_pinmux {
	smartfarm_gpio_pins: pinmux_smartfarm_gpio_pins {
        pinctrl-single,pins = <
            AM33XX_PADCONF(AM335X_PIN_GPMC_ADVN_ALE, PIN_OUTPUT_PULLDOWN, MUX_MODE7)	/* P8_7 */
            AM33XX_PADCONF(AM335X_PIN_GPMC_OEN_REN, PIN_OUTPUT_PULLDOWN, MUX_MODE7)		/* P8_8 */
        >;
    };
};

&gpio2 {
    smartfarm_hog {
        pinctrl-names = "default";
        pinctrl-0 = <&smartfarm_gpio_pins>;

        gpio-hog;
        gpios = <2 GPIO_ACTIVE_HIGH>, <3 GPIO_ACTIVE_HIGH>;
        output-low;              /* mặc định kéo xuống */
        line-name = "smartfarm";
    };
};
```

Khi kernel runtime, sử dụng lệnh `gpio-info` thì nó ra như sau:

```
gpiochip1 - 32 lines:
        line   0:     "P9_15B"       unused   input  active-high
        line   1:      "P8_18"       unused   input  active-high
        line   2:       "P8_7"  "smartfarm"  output  active-high [used]
        line   3:       "P8_8"  "smartfarm"  output  active-high [used]
gpiochip2 - 32 lines:
        line   0:  "[mii col]"       unused   input  active-high
        line   1:  "[mii crs]"       unused   input  active-high
        line   2: "[mii rx err]" "light" output active-high [used]
        line   3: "[mii tx en]" "pump" output active-high [used]
```

Ở đây, nhận thấy rắng nó request 2 chân gpio P8_7 và P8_8. Nhưng, chú ý thấy tại gpiochip2 thì nó xuất hiện hai gpio pump và light mà trong code thì mình đang request gpio là P8_7 VÀ p8_8. 

Sau một hồi tra rõ thì gpiochip ở runtime nó bị lệch bank so với khi được cấu hình ở device tree như sau:

```
&gpio2 {
	gpio-line-names =
		"P9_15B",
		"P8_18",
		"P8_7",
		"P8_8",
		"P8_10",
		"P8_9",
		"P8_45 [hdmi]",
		"P8_46 [hdmi]",
		"P8_43 [hdmi]",
		"P8_44 [hdmi]",
		"P8_41 [hdmi]",
		"P8_42 [hdmi]",
		"P8_39 [hdmi]",
		"P8_40 [hdmi]",
		"P8_37 [hdmi]",
		"P8_38 [hdmi]",
		"P8_36 [hdmi]",
		"P8_34 [hdmi]",
		"[rmii1_rxd3]",
		"[rmii1_rxd2]",
		"[rmii1_rxd1]",
		"[rmii1_rxd0]",
		"P8_27 [hdmi]",
		"P8_29 [hdmi]",
		"P8_28 [hdmi]",
		"P8_30 [hdmi]",
		"[mmc0_dat3]",
		"[mmc0_dat2]",
		"[mmc0_dat1]",
		"[mmc0_dat0]",
		"[mmc0_clk]",
		"[mmc0_cmd]";
};

&gpio3 {
	gpio-line-names =
		"[mii col]",
		"[mii crs]",
		"[mii rx err]",
		"[mii tx en]",
		"[mii rx dv]",
		"[i2c0 sda]",
		"[i2c0 scl]",
		"[jtag emu0]",
		"[jtag emu1]",
		"[mii tx clk]",
		"[mii rx clk]",
		"NC",
		"NC",
		"[usb vbus en]",
		"P9_31 [spi1_sclk]",
		"P9_29 [spi1_d0]",
		"P9_30 [spi1_d1]",
		"P9_28 [spi1_cs0]",
		"P9_42B [ecappwm0]",
		"P9_27",
		"P9_41A",
		"P9_25",
		"NC",
		"NC",
		"NC",
		"NC",
		"NC",
		"NC",
		"NC",
		"NC",
		"NC",
		"NC";
};
```

Nhìn thì ta thấy gpio2 -> gpiochip1, gpio3 -> gpiochip2. Nguyên nhân của việc này là do thứ tự driver register lúc boot.

=> Cần chỉnh đúng gpiochip2 -> gpiochip1.