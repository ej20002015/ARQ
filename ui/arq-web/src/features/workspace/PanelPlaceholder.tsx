export function PanelPlaceholder(props: any) {
    const title = props.params.title || "Placeholder Panel";
    return (
        <div className="flex h-full w-full flex-col items-center justify-center bg-neutral-950 text-neutral-200 p-4 select-none">
            <div className="text-center">
                <p className="text-lg font-semibold">{title}</p>
                <p className="text-xs text-neutral-500 font-mono mt-1">ID: {props.api.id}</p>
            </div>
        </div>
    );
}