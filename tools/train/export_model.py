from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np


INPUT_SIZE = 28 * 28
CLASS_COUNT = 10


def scalar_str(data: dict[str, np.ndarray], key: str) -> str:
    return str(data[key].tolist())


def scalar_int(data: dict[str, np.ndarray], key: str) -> int:
    return int(data[key].tolist())


def format_float(value: float) -> str:
    return f"{float(value):.8e}f"


def quantize_symmetric(values: np.ndarray) -> tuple[np.ndarray, float]:
    max_abs = float(np.max(np.abs(values))) if values.size else 0.0
    scale = max_abs / 127.0 if max_abs > 0.0 else 1.0
    quantized = np.clip(np.round(values / scale), -127, 127).astype(np.int8)
    return quantized, scale


def c_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_matrix(f, name: str, rows: int, cols: int, values: np.ndarray) -> None:
    f.write(f"const float {name}[{rows}][{cols}] = {{\n")
    for row in range(rows):
        f.write("    {\n")
        for offset in range(0, cols, 8):
            line = ", ".join(format_float(v) for v in values[row, offset : offset + 8])
            f.write(f"        {line},\n")
        f.write("    },\n")
    f.write("};\n\n")


def write_matrix_i8(f, name: str, rows: int, cols: int, values: np.ndarray) -> None:
    f.write(f"const int8_t {name}[{rows}][{cols}] = {{\n")
    for row in range(rows):
        f.write("    {\n")
        for offset in range(0, cols, 16):
            line = ", ".join(str(int(v)) for v in values[row, offset : offset + 16])
            f.write(f"        {line},\n")
        f.write("    },\n")
    f.write("};\n\n")


def write_conv4d(f, name: str, shape: tuple[int, int, int, int], values: np.ndarray) -> None:
    out_c, in_c, kernel_h, kernel_w = shape
    f.write(f"const float {name}[{out_c}][{in_c}][{kernel_h}][{kernel_w}] = {{\n")
    for oc in range(out_c):
        f.write("    {\n")
        for ic in range(in_c):
            f.write("        {\n")
            for ky in range(kernel_h):
                row = ", ".join(format_float(v) for v in values[oc, ic, ky])
                f.write(f"            {{{row}}},\n")
            f.write("        },\n")
        f.write("    },\n")
    f.write("};\n\n")


def write_conv4d_i8(f, name: str, shape: tuple[int, int, int, int], values: np.ndarray) -> None:
    out_c, in_c, kernel_h, kernel_w = shape
    f.write(f"const int8_t {name}[{out_c}][{in_c}][{kernel_h}][{kernel_w}] = {{\n")
    for oc in range(out_c):
        f.write("    {\n")
        for ic in range(in_c):
            f.write("        {\n")
            for ky in range(kernel_h):
                row = ", ".join(str(int(v)) for v in values[oc, ic, ky])
                f.write(f"            {{{row}}},\n")
            f.write("        },\n")
        f.write("    },\n")
    f.write("};\n\n")


def write_vector(f, name: str, size: int, values: np.ndarray) -> None:
    f.write(f"const float {name}[{size}] = {{\n    ")
    f.write(", ".join(format_float(v) for v in values))
    f.write("\n};\n\n")


def write_header(path: Path, model_type: str, model_name: str, hidden_size: int, quant: str) -> None:
    if model_type == "mlp":
        model_type_macro = "MODEL_TYPE_MLP"
    elif model_type == "cnn":
        model_type_macro = "MODEL_TYPE_CNN"
    else:
        model_type_macro = "MODEL_TYPE_PERCEPTRON"
    deploy_model_name = f"{model_name}-{quant}" if quant != "fp32" else model_name
    path.write_text(
        f"""#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

#define MODEL_TYPE_PERCEPTRON 1
#define MODEL_TYPE_MLP 2
#define MODEL_TYPE_CNN 3
#define MODEL_QUANT_FP32 0
#define MODEL_QUANT_INT8 1

#define MODEL_INPUT_SIZE 784U
#define MODEL_CLASS_COUNT 10U
#define MODEL_HIDDEN_SIZE {hidden_size}U
#define MODEL_CONV1_OUT 8U
#define MODEL_CONV2_OUT 16U
#define MODEL_CNN_FC_INPUT_SIZE 784U
#define MODEL_TYPE {model_type_macro}
#define MODEL_QUANT_TYPE {"MODEL_QUANT_INT8" if quant == "int8" else "MODEL_QUANT_FP32"}
#define MODEL_NAME {c_string(deploy_model_name)}
#define MODEL_TYPE_NAME {c_string(model_type)}
#define MODEL_QUANT_NAME {c_string(quant)}
#define MODEL_GENERATED 1

#if MODEL_TYPE == MODEL_TYPE_CNN && MODEL_QUANT_TYPE == MODEL_QUANT_INT8
extern const int8_t g_model_conv1_weights_q[MODEL_CONV1_OUT][1][3][3];
extern const float g_model_conv1_weight_scale;
extern const float g_model_conv1_bias[MODEL_CONV1_OUT];
extern const int8_t g_model_conv2_weights_q[MODEL_CONV2_OUT][MODEL_CONV1_OUT][3][3];
extern const float g_model_conv2_weight_scale;
extern const float g_model_conv2_bias[MODEL_CONV2_OUT];
extern const int8_t g_model_fc_weights_q[MODEL_CLASS_COUNT][MODEL_CNN_FC_INPUT_SIZE];
extern const float g_model_fc_weight_scale;
extern const float g_model_fc_bias[MODEL_CLASS_COUNT];
#elif MODEL_TYPE == MODEL_TYPE_CNN
extern const float g_model_conv1_weights[MODEL_CONV1_OUT][1][3][3];
extern const float g_model_conv1_bias[MODEL_CONV1_OUT];
extern const float g_model_conv2_weights[MODEL_CONV2_OUT][MODEL_CONV1_OUT][3][3];
extern const float g_model_conv2_bias[MODEL_CONV2_OUT];
extern const float g_model_fc_weights[MODEL_CLASS_COUNT][MODEL_CNN_FC_INPUT_SIZE];
extern const float g_model_fc_bias[MODEL_CLASS_COUNT];
#elif MODEL_TYPE == MODEL_TYPE_MLP
extern const float g_model_hidden_weights[MODEL_HIDDEN_SIZE][MODEL_INPUT_SIZE];
extern const float g_model_hidden_bias[MODEL_HIDDEN_SIZE];
extern const float g_model_output_weights[MODEL_CLASS_COUNT][MODEL_HIDDEN_SIZE];
extern const float g_model_output_bias[MODEL_CLASS_COUNT];
#else
extern const float g_model_output_weights[MODEL_CLASS_COUNT][MODEL_INPUT_SIZE];
extern const float g_model_output_bias[MODEL_CLASS_COUNT];
#endif

#endif /* MODEL_DATA_H */
""",
        encoding="utf-8",
    )


def write_source(path: Path, data: dict[str, np.ndarray], model_type: str, hidden_size: int, quant: str) -> None:
    with path.open("w", encoding="utf-8") as f:
        f.write('#include "ModelData.h"\n\n')

        if model_type == "mlp":
            output_weights = data["output_weights"].astype(np.float32)
            output_bias = data["output_bias"].astype(np.float32)
            hidden_weights = data["hidden_weights"].astype(np.float32)
            hidden_bias = data["hidden_bias"].astype(np.float32)
            if hidden_weights.shape != (hidden_size, INPUT_SIZE):
                raise ValueError(f"unexpected hidden weight shape: {hidden_weights.shape}")
            if hidden_bias.shape != (hidden_size,):
                raise ValueError(f"unexpected hidden bias shape: {hidden_bias.shape}")
            if output_weights.shape != (CLASS_COUNT, hidden_size):
                raise ValueError(f"unexpected output weight shape: {output_weights.shape}")
            write_matrix(f, "g_model_hidden_weights", hidden_size, INPUT_SIZE, hidden_weights)
            write_vector(f, "g_model_hidden_bias", hidden_size, hidden_bias)
            write_matrix(f, "g_model_output_weights", CLASS_COUNT, hidden_size, output_weights)
            if output_bias.shape != (CLASS_COUNT,):
                raise ValueError(f"unexpected output bias shape: {output_bias.shape}")
            write_vector(f, "g_model_output_bias", CLASS_COUNT, output_bias)
        elif model_type == "cnn":
            conv1_weights = data["conv1_weights"].astype(np.float32)
            conv1_bias = data["conv1_bias"].astype(np.float32)
            conv2_weights = data["conv2_weights"].astype(np.float32)
            conv2_bias = data["conv2_bias"].astype(np.float32)
            fc_weights = data["fc_weights"].astype(np.float32)
            fc_bias = data["fc_bias"].astype(np.float32)
            if conv1_weights.shape != (8, 1, 3, 3):
                raise ValueError(f"unexpected conv1 weight shape: {conv1_weights.shape}")
            if conv1_bias.shape != (8,):
                raise ValueError(f"unexpected conv1 bias shape: {conv1_bias.shape}")
            if conv2_weights.shape != (16, 8, 3, 3):
                raise ValueError(f"unexpected conv2 weight shape: {conv2_weights.shape}")
            if conv2_bias.shape != (16,):
                raise ValueError(f"unexpected conv2 bias shape: {conv2_bias.shape}")
            if fc_weights.shape != (CLASS_COUNT, INPUT_SIZE):
                raise ValueError(f"unexpected fc weight shape: {fc_weights.shape}")
            if fc_bias.shape != (CLASS_COUNT,):
                raise ValueError(f"unexpected fc bias shape: {fc_bias.shape}")
            if quant == "int8":
                conv1_q, conv1_scale = quantize_symmetric(conv1_weights)
                conv2_q, conv2_scale = quantize_symmetric(conv2_weights)
                fc_q, fc_scale = quantize_symmetric(fc_weights)
                write_conv4d_i8(f, "g_model_conv1_weights_q", (8, 1, 3, 3), conv1_q)
                f.write(f"const float g_model_conv1_weight_scale = {format_float(conv1_scale)};\n\n")
                write_vector(f, "g_model_conv1_bias", 8, conv1_bias)
                write_conv4d_i8(f, "g_model_conv2_weights_q", (16, 8, 3, 3), conv2_q)
                f.write(f"const float g_model_conv2_weight_scale = {format_float(conv2_scale)};\n\n")
                write_vector(f, "g_model_conv2_bias", 16, conv2_bias)
                write_matrix_i8(f, "g_model_fc_weights_q", CLASS_COUNT, INPUT_SIZE, fc_q)
                f.write(f"const float g_model_fc_weight_scale = {format_float(fc_scale)};\n\n")
                write_vector(f, "g_model_fc_bias", CLASS_COUNT, fc_bias)
            else:
                write_conv4d(f, "g_model_conv1_weights", (8, 1, 3, 3), conv1_weights)
                write_vector(f, "g_model_conv1_bias", 8, conv1_bias)
                write_conv4d(f, "g_model_conv2_weights", (16, 8, 3, 3), conv2_weights)
                write_vector(f, "g_model_conv2_bias", 16, conv2_bias)
                write_matrix(f, "g_model_fc_weights", CLASS_COUNT, INPUT_SIZE, fc_weights)
                write_vector(f, "g_model_fc_bias", CLASS_COUNT, fc_bias)
        else:
            output_weights = data["output_weights"].astype(np.float32)
            output_bias = data["output_bias"].astype(np.float32)
            if output_weights.shape != (CLASS_COUNT, INPUT_SIZE):
                raise ValueError(f"unexpected output weight shape: {output_weights.shape}")
            write_matrix(f, "g_model_output_weights", CLASS_COUNT, INPUT_SIZE, output_weights)
            if output_bias.shape != (CLASS_COUNT,):
                raise ValueError(f"unexpected output bias shape: {output_bias.shape}")
            write_vector(f, "g_model_output_bias", CLASS_COUNT, output_bias)


def main() -> None:
    parser = argparse.ArgumentParser(description="Export perceptron or MLP model to STM32 C arrays.")
    parser.add_argument("weights", help="model.npz generated by train_model.py")
    parser.add_argument("--out-dir", default=None, help="Directory for ModelData.c/h")
    parser.add_argument("--quant", choices=["fp32", "int8"], default="fp32", help="Quantization format for export")
    args = parser.parse_args()

    npz = np.load(args.weights)
    data = {key: npz[key] for key in npz.files}
    if "model_type" not in data:
        if "weights" not in data or "bias" not in data:
            raise ValueError("legacy perceptron export requires weights and bias arrays")
        data = {
            "model_type": np.array("perceptron"),
            "model_name": np.array("perceptron"),
            "input_size": np.int32(INPUT_SIZE),
            "class_count": np.int32(CLASS_COUNT),
            "hidden_size": np.int32(0),
            "output_weights": data["weights"].astype(np.float32),
            "output_bias": data["bias"].astype(np.float32),
        }

    model_type = scalar_str(data, "model_type")
    model_name = scalar_str(data, "model_name")
    hidden_size = scalar_int(data, "hidden_size")

    if scalar_int(data, "input_size") != INPUT_SIZE:
        raise ValueError("model input size is not 784")
    if scalar_int(data, "class_count") != CLASS_COUNT:
        raise ValueError("model class count is not 10")
    if model_type not in {"perceptron", "mlp", "cnn"}:
        raise ValueError(f"unsupported model type: {model_type}")
    if args.quant == "int8" and model_type != "cnn":
        raise ValueError("int8 export is currently implemented for cnn models")

    repo_root = Path(__file__).resolve().parents[2]
    out_dir = Path(args.out_dir).resolve() if args.out_dir else repo_root / "User" / "model"
    out_dir.mkdir(parents=True, exist_ok=True)
    write_header(out_dir / "ModelData.h", model_type, model_name, hidden_size, args.quant)
    write_source(out_dir / "ModelData.c", data, model_type, hidden_size, args.quant)
    print(f"exported {model_name} ({model_type}, {args.quant}) to {out_dir}")


if __name__ == "__main__":
    main()
