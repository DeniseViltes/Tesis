def unidades_originales(df):
    """Recupera A y J de CSV antiguos en mA y mJ, sin modificar el archivo."""
    df = df.copy()
    for column in list(df.columns):
        if column.endswith("_mA") or column.endswith("_mJ"):
            original = column[:-2] + column[-1]
            if original not in df.columns:
                df[original] = df[column] / 1000.0
            df = df.drop(columns=[column])
    return df
