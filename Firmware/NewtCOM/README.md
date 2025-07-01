```
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
```

# NewtCOM Dongle configuration

This is the build configuration for running the NewtCOM firmware on a 
NewtCOM Dongle with an RP2040 CPU. The firmware installes to Flash memory.

Parallel to this configuration is the PiPico setup for compiling the 
same firmware for the PiPico developer board.

## My TODO List for NewtCOM Dongle

see: [../PiPico/README.txt](../PiPico/README.txt)

Also:
 - [ ] User Documentation, reference to the resources that we use

 ## Hardware

 The pack is built with 10.5mm long pin headers, giving a top-bottom distance
 of 9.1 to 9.5mm. The overall hight is 12.5mm without USB and 13.9mm with USB
 connector. 
 
 Tip of the plug to front edge of PCBs is 10.4mm. Length over all is 31mm and
 32.5mm including the USB port.

 Width is 17.8mm. 

 The original dongle is 21.4 to 24.7mm wide, 20mm tall, and 39.4mm long 
 with Interconnect and 32,4mm without.

## Staus Report

The dongle can pipe data from the serial port to the USB port and back reliably at 115200. Various test were done with NCX and BasiliskII and 
Inspector.

The Dock protocol is implemented well enough so the Newton can establish
a connection to an emulated NCU, browse files and directories on the 
SDCard, and download and install them.


## GitHub

Task to build PiPico stuff on GitHub:

https://github.com/raspberrypi/pico-examples/blob/master/.github/workflows/macOS.yml



To drop **7.5V to \~7.2V** using two **series diodes**, you're doing a very common trick — using the **forward voltage drop** of the diodes (\~0.3V total) to bring the voltage into spec. Let’s walk through the choices and protection details.

---

### 🔧 Diode Selection

Since you're passing **up to 1A** of current, you need:

* A diode with **low forward voltage** (typically 0.2–0.4V).
* A diode that can handle **at least 1A continuous current**.
* A **common SMD package** for ease of assembly.

#### ✅ Recommended SMD Diodes

| Diode        | Type          | Package            | Vf @ 1A                | Max I                  | Notes                                     |
| ------------ | ------------- | ------------------ | ---------------------- | ---------------------- | ----------------------------------------- |
| **SS14**     | Schottky      | SMA                | \~0.4V                 | 1A (Avg) / 30A (surge) | Very common, cheap, ideal for power rails |
| **SS34**     | Schottky      | DO-214AB (SMA/SMB) | \~0.4V                 | 3A                     | More margin, same voltage drop            |
| **MBRS130L** | Schottky      | SMB                | \~0.3V                 | 1A                     | Lower Vf than SS14, also common           |
| **BAT54C**   | Dual Schottky | SOT-23             | \~0.3V @ lower current | 200mA per diode        | Too small for 1A total                    |

For your case:
👉 **Use two SS14s or MBRS130Ls in series**.
They’re widely available, cheap, and reliable.

---

### ⚠️ Do You Need a Fuse?

Yes, **some form of protection is wise**. DC-DC converters can fail shorted or supply too much current in a fault scenario. You want to prevent overheating or damage.

#### 🔌 Options for Electronic "Fusing":

1. **Resettable Polyfuse (PTC)**

   * Simple, self-resetting.
   * Choose one rated for **1.1–1.5A hold**.
   * Example: **MF-MSMF110** (1.1A hold, 2.2A trip).
   * SMD versions available.

2. **SMD Fuse (non-resettable)**

   * More compact and predictable.
   * Choose \~1.5A fuse to protect the 1A line.
   * Littelfuse 045101.5MRL (1206, 1.5A).

3. **Electronic Fuse IC**

   * Active current limiter (e.g., TPS25940 or similar).
   * Overkill unless you're designing for high reliability.

For most DIY or low-volume boards:
👉 **A polyfuse + TVS diode** offers good balance between protection and simplicity.

---

### 🛡️ Bonus: Add a TVS Diode for Transients

Put a **TVS diode** on the 7.2V output to clamp voltage spikes.

* For 7.2V nominal, use something like **SMBJ8.0A** (clamps at \~13V, starts at 8.9V).
* Pick one based on how sensitive your load is.

---

### ✅ Summary

* **Use 2x SS14 or MBRS130L diodes in series** to drop 0.3V.
* **Add a resettable fuse** (1.1–1.5A) or SMD fuse on the 7.5V or 7.2V line.
* **Optional but recommended:** TVS diode to protect from spikes.

Let me know what your load is — I can help tune the exact diode and protection setup.


Thanks — the Apple **Newton** is indeed **very sensitive to overvoltage**, especially on the external power input, as it was designed around linear regulators and NiCd/NiMH charging circuits. Given that:

---

### 🚨 Key Constraints

* **Never exceed 7.3V** (absolute max).
* Nominal target: **7.2V ± 0.1V**.
* Current draw: typically **<100mA**, but **may spike to 1A** when charging.
* Load is **delicate**, so reliability and protection matter more than just basic functionality.

---

### ✅ Best Diode Setup

To drop 7.5V to \~7.2V:

#### Recommended diode:

* **MBRS130L (SMB)** or **1N5819-SMD**

  * Forward drop: \~0.35–0.4V at 1A, less at lower current.
  * Current rating: 1A continuous.
  * Soft turn-on, minimal overshoot.

These will each drop around **0.2–0.3V at 100mA**, or **0.35–0.4V at 1A**. Two in series should bring 7.5V down to \~7.2V or slightly below under load — **perfect** for your case.

✅ Use: **2x MBRS130L diodes in series** in the main power path.

---

### 🔐 Protection: You Absolutely Need a Fuse and Clamp

Overvoltage = instant damage to the Newton. You want **two layers of defense**:

#### 1. **Fuse or Polyfuse**

* Use a **fast-acting SMD fuse**, like:

  * **Littelfuse 0451.125MRL** – 125mA
  * **0452.500MRL** – 500mA for some charging margin
* Or a **polyfuse**, like:

  * **MF-MSMF050-2**: 0.5A hold, \~1A trip
  * **MF-MSMF075-2**: 0.75A hold, \~1.5A trip

A polyfuse is slower but self-resetting — more forgiving.

#### 2. **TVS Diode (Clamp)**

Place a **TVS diode across the Newton’s supply input**, just before it connects to the device:

* Use **SMBJ7.0A** or **SMBJ6.5A**:

  * Breakdown: \~7.0V
  * Clamps: \~11.3V (SMBJ7.0A) or \~10.5V (SMBJ6.5A)
  * Both will prevent a >7.5V surge from reaching the Newton.

> **Note:** A Zener diode is *not fast enough* for this. Use a **TVS (transient voltage suppressor)**.

---

### 🧱 Suggested Power Path Summary

```text
[USB-C 5V] ─> [Boost Converter to 7.5V] ─> [Fuse or Polyfuse]
                                               │
                                ┌──────────────┘
                          [2x Schottky Diodes (SS14 or MBRS130L)]
                                               │
                                       [TVS Diode to GND]
                                               │
                                          [Newton Input]
```

---

### ⚙️ Extra Tips

* Test under load with multimeter: the forward drop **will vary** from \~0.25V (100mA) to \~0.4V (1A).
* Make sure the **boost converter is well filtered** — Newtons are also sensitive to ripple.
* You might consider a **low-dropout linear regulator** (e.g., **MIC29300-7.5**), but diodes are easier and smaller for this use case.

---

Would you like me to suggest exact part numbers and footprints for your PCB?



Perfect — here’s a clean, **low-part-count and safe design** for powering the Apple Newton from a USB-C supply, with **all the exact SMD part numbers** and footprints you can drop into your PCB layout.

---

## 🧩 Final BOM & Footprints

### 1. **DC-DC Boost Converter (5V to 7.5V)**

* You already have this — just ensure it’s:

  * Capable of stable 7.5V out @ 1–1.5A.
  * Has low ripple (<50mV) and soft start.

---

### 2. **Fuse / Polyfuse**

#### Option A: *Resettable Polyfuse* (recommended for gentle protection)

* **Part:** [Littelfuse MF-MSMF050-2](https://www.littelfuse.com/products/resettable-ptcs/surface-mount/mf-msmf-series/mf-msmf050-2.aspx)
* **Rating:** 0.5A hold / 1A trip
* **Footprint:** 1206 (3216 metric)

#### Option B: *SMD Fuse* (faster, non-resettable)

* **Part:** [Littelfuse 0452005.MRL](https://www.littelfuse.com/products/fuses/surface-mount-fuses/pico-smd/0452/0452005.MRL.aspx)
* **Rating:** 500mA fast-blow
* **Footprint:** 1206 (3216 metric)

---

### 3. **Series Diodes (for voltage drop)**

* **Part:** [MBRS130L-TP](https://www.diodes.com/assets/Datasheets/ds13001.pdf)
* **Rating:** 1A avg, 0.35V drop at 1A
* **Footprint:** SMB (DO-214AA)

💡 You’ll use **2x in series**. Place them with current flowing **from boost converter to Newton input**.

Alternate: [SS14](https://www.onsemi.com/pdf/datasheet/ss14-d.pdf) — also works, slightly higher Vf.

---

### 4. **TVS Diode (Overvoltage Clamp)**

* **Part:** [SMBJ7.0A](https://www.littelfuse.com/products/tvs-diodes/smbj/smbj7-0a.aspx)
* **Rating:** 7.0V breakdown, clamps at \~11.3V
* **Footprint:** SMB (DO-214AA)

💡 This protects the Newton against spikes or DC/DC runaway. Connect from **7.2V rail to GND**.

Alternate (tighter clamp): **SMBJ6.5A** – breaks down at 6.5V, clamps around 10.5V, but may trip under brief surges.

---

## 🧾 PCB Layout Notes

* Place **TVS diode** physically **close to Newton input pin**.
* Route **diodes** and **fuse** in series on the high side (not ground).
* Keep all high-current paths **short and wide** (>=40 mil trace if possible).
* Add **10µF ceramic + 100nF ceramic** bypass caps at Newton input to reduce ripple and absorb switching noise.

---

## 💡 Summary

| Function        | Part Number  | Package | Value/Rating         |
| --------------- | ------------ | ------- | -------------------- |
| Resettable Fuse | MF-MSMF050-2 | 1206    | 0.5A hold / 1A trip  |
| Diodes (x2)     | MBRS130L-TP  | SMB     | 0.35V drop @ 1A each |
| TVS Clamp       | SMBJ7.0A     | SMB     | 7.0V breakdown       |

Would you like me to generate a KiCad footprint library for these parts or an example schematic layout?
