export function formatDecimal(value: any, decimalPlaces: number): string {
    if (value == null || value === "")
        return "";

    const num = Number(value);
    if (isNaN(num))
        return String(value);

    return new Intl.NumberFormat("en-US", {
        minimumFractionDigits: decimalPlaces,
        maximumFractionDigits: decimalPlaces,
    }).format(num);
}

export function formatPercentage(value: any): string {
    if (value == null || value === "")
        return "";

    const num = Number(value);
    if (isNaN(num))
        return String(value);

    // The "percent" style automatically multiplies by 100 and adds the "%" sign
    return new Intl.NumberFormat("en-US", {
        style: "percent",
        minimumFractionDigits: 2,
        maximumFractionDigits: 2,
    }).format(num);
}

export function formatBps(value: any): string {
    if (value == null || value === "")
        return "";

    const num = Number(value);
    if (isNaN(num))
        return String(value);

    // 1 basis point = 0.0001
    const bps = num * 10000;
    return `${new Intl.NumberFormat("en-US", { maximumFractionDigits: 0 }).format(bps)} bps`;
}

export function formatDate(value: any): string {
    if (!value)
        return "";

    const date = new Date(value);
    return isNaN(date.getTime()) ? String(value) : date.toLocaleDateString("en-GB");
}

export function formatDateTime(value: any): string {
    if (!value)
        return "";

    const date = new Date(value);
    return isNaN(date.getTime()) ? String(value) : date.toLocaleString("en-GB");
}