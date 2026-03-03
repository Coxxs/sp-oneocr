# OneOCR for Snipaste

Use Windows Snipping Tool's offline OCR (OneOCR) in Snipaste.

## Usage

Snipaste PRO is required.

1. Download the latest release of [sp-oneocr](https://github.com/Coxxs/sp-oneocr/releases), and extract it.
   - Install [Microsoft Visual C++ Redistributable v14 (x64)](https://aka.ms/vc14/vc_redist.x64.exe) if not already installed.

2. Run `ExtractOCR.bat` to extract the required OCR files from Snipping Tool.

   > If Snipping Tool is not found, [install it](https://apps.microsoft.com/detail/9mz95kl8mr0l) from the Microsoft Store first.

   <details>
   <summary>Alternative: Copy files manually</summary>

   Copy the following files to the same folder as `sponeocr.exe`:
   - `oneocr.dll`
   - `onnxruntime.dll`
   - `oneocr.onemodel`

   You can find these files in the Snipping Tool installation folder, e.g.:<br>
   `C:\Program Files\WindowsApps\Microsoft.ScreenSketch_11.2510.31.0_x64__8wekyb3d8bbwe\SnippingTool\`<br>
   Note: `11.2510.31.0` is the version of Snipping Tool.

   Alternatively, you can download these files [here](https://archive.org/download/microsoft.-screen-sketch-2022.2510.31.0-neutral-8wekyb-3d-8bbwe/OneOCR.zip).
   </details>

3. Configure Snipaste:
   - Go to **Snipaste** -> **Preferences...** -> **Output** -> **Text Recognition**
   - Set **OCR Engine** to `Tesseract`
   - Set **Executable** to the path of `sponeocr.exe`
