# Egypt sourcing review — 4 November 2026

Owner preferred suppliers: RAM Electronics, Flux Electronix, Lampatronics,
Future Electronics Egypt and Micro Ohm. These are preferences, not purchase orders.
Online listings are discovery evidence; stock and module identity need confirmation.
No electrical part is released solely on its advertised current rating.

## Initial screening

| Listed item | Disposition for current single-2S vehicle |
|---|---|
| RAM DC107 HW-688 5 V/5 A module | Reject for direct 2S input: listing specifies 9–36 V input; full 2S is only 8.4 V. |
| RAM DC110 MINI560 fixed 5 V/5 A | Candidate for investigation only. Obtain actual IC/module documentation and continuous-load/dropout/thermal evidence; catalog stock indications conflict. |
| RAM XL4015/XL4016 modules | Hold. Catalog input claims vary; exact board, IC documentation and low-input full-load operation must be established. Larger advertised current is not proof of suitability. |
| Lampatronics 300 W module, listed 7–32 V input | Does not cover the proposed 6.4–8.4 V qualification envelope; do not adopt unchanged. |
| Micro Ohm 5 V/5 A SMPS | Reject for onboard battery use: listing specifies AC mains input. |
| Micro Ohm 2S 4 A BMS | Reject as whole-vehicle main protection: below the existing 13.66 A average planning current, before peaks. |
| Future 11.1 V 8000 mAh LiPo | Not a substitute for the selected 2S architecture; it is a 3S pack. |
| Exact 2S 9000 mAh RC pack | Not verified in reviewed listings. Absence from this search is not evidence shops cannot supply it. |
| Flux suitable regulator/protection/battery | No specific suitable listing verified in this pass. |

The initial review does not establish a locally stocked, fully qualified regulator,
cutoff/OVP assembly or battery. Keep U1/U3 and disconnect selection open. Do not replace
the single-2S design with 3S just to suit a convenient converter.

## Supplier enquiry specification (draft for owner to use; not sent)

Please provide exact model, datasheet, board photos and availability for:

1. A DC/DC supply delivering regulated 5 V at 5 A continuous with thermal margin,
   covering 6.4–8.4 V input under load; include dropout, ripple and protections.
   Actual battery cutoff remains to be selected; 6.4 V is a qualification endpoint.
2. Standard 2S 7.4 V LiPo, target 9000 mAh, with dimensions, weight, connector,
   manufacturer continuous/burst current ratings and compatible balance charger.
3. Per-cell 2S monitoring and a documented DC load disconnect suitable for the
   traction current envelope, plus a way to warn/shut down the Pi before cutoff.
   Final current rating requires motor/servo transient evidence.
4. Documented 5 V overvoltage protection and a secured Pi power carrier rated for
   the required load, with thresholds, fault response and connector specifications.

## Sources checked

- RAM converter catalog: https://www.ram.com.eg/shop?order=list_price+asc&search=Dc.dc
- RAM DC107: https://www.ram.com.eg/shop/dc-dc-107-dc-dc-step-down-voltage-voltage-converter-5a-24v-12v-to-fixed-output-5v-5a-sku-dc107-8203
- Lampatronics supply catalog: https://lampatronics.com/product?categories=356
- Micro Ohm SMPS: https://microohm-eg.com/power-supply-5v-5a-smps/
- Micro Ohm catalog (2S 4 A BMS listing): https://microohm-eg.com/shop/page/25/
- Future battery catalog: https://store.fut-electronics.com/collections/battery-solar-relay-supply
- Flux storefront identified: https://fluxelectronix.com/

Supplier data are not manufacturer qualification. XL Semiconductor datasheet access
failed during this review; no unverified IC minimum-input claim is used as approval.
