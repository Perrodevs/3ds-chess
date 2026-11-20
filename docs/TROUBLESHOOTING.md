# Troubleshooting

| Symptom | What to check |
| --- | --- |
| `atlas.hpp` is missing | Generate the original board art with `make assets`. |
| `portraits.hpp` is missing | The repository does not include the mixed portrait sheets. Follow the [build guide](BUILDING.md) using authorized inputs. |
| devkitARM or Citro2D is missing | Verify `DEVKITPRO` and `DEVKITARM` and install the 3DS SDK dependencies. |
| The game opens without audio | Check whether NDSP is available. The game can continue silently when initialization fails. |
| Profile creation is blocked on 1.4.x | Record the build, exact action and displayed code. In RC2, try the suggested-name route if the software keyboard cannot open. Back up the SD data before attempting recovery. |

On 1.4.0 diagnostic candidates, logs live at
`SD:/3ds/3ds-chess/logs/session.log` and `session.previous.log`. Review only
the relevant session and remove identifying data before sharing. The
[stability report](https://github.com/Perrodevs/3ds-chess/blob/fix/stability-quality-update/docs/STABILITY_140_RC2.md)
documents the ARM startup error fixed in RC2. The Legacy default branch lacks
this diagnostic journal. Use the [bug form](../.github/ISSUE_TEMPLATE/bug-report.yml)
for non-sensitive issues.
