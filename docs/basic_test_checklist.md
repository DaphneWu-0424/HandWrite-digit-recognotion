# Basic Handwritten Digit Test Checklist

Use this after exporting a trained `ModelData.c/h` model and flashing the board.

## Board Test

- Power on: LCD shows the write area, preview area, result text, and `Clear` button.
- Draw area: writing inside the boxed area leaves a visible stroke.
- Release behavior: lifting the pen/finger updates the 28x28 preview and result.
- Clear behavior: tapping `Clear` resets the canvas and allows a new digit.

## Accuracy Notes

Record at least three samples per digit for the basic task.

| Digit | Sample 1 | Sample 2 | Sample 3 | Notes |
| --- | --- | --- | --- | --- |
| 0 |  |  |  |  |
| 1 |  |  |  |  |
| 2 |  |  |  |  |
| 3 |  |  |  |  |
| 4 |  |  |  |  |
| 5 |  |  |  |  |
| 6 |  |  |  |  |
| 7 |  |  |  |  |
| 8 |  |  |  |  |
| 9 |  |  |  |  |

If results are poor, adjust preprocessing first: stroke thickness, crop box, centering, and scaling.
