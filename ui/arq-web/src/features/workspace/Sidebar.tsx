import { Database, Table, Search, ChartCandlestick, ChevronLeft, ChevronRight } from "lucide-react";
import { Button } from '@/components/ui/button';
import arqLogo from "../../assets/arqLogoSideup.svg";

interface SidebarButtonProps {
    icon: React.ReactNode;
    label: string;
    isOpen: boolean;
    onClick: () => void;
}

function SidebarButton({icon, label, isOpen, onClick }: SidebarButtonProps) {
    return (
        <Button
            variant="ghost"
            onClick={onClick}
            className={`w-full overflow-hidden transition-colors text-muted-foreground hover:bg-primary/10 hover:text-primary ${
                isOpen ? "justify-start px-2 gap-3" : "justify-center px-0"
            }`}
        >
            <div className="min-w-[20px] flex justify-center">{icon}</div>
            {isOpen && <span>{label}</span>}
        </Button>
    );
}

interface SidebarProps {
    isOpen: boolean;
    onToggle: () => void;
    onOpenPanel: (id: string, title: string, componentName?: string) => void;
    onOpenCmdk: () => void;
}

export function Sidebar({ isOpen, onToggle, onOpenPanel, onOpenCmdk }: SidebarProps) {
    return (
        <div className={`transition-all duration-300 ease-in-out border-r border-border bg-background flex flex-col ${isOpen ? "w-60" : "w-16"}`}>

            {/* Header & Toggle Button */}
            <div className={`flex h-14 items-center justify-between px-4 border-b border-border ${!isOpen && "justify-center"}`}>
                
                {isOpen && (
                    <img 
                        src={arqLogo}
                        alt="ARQ"
                        className="h-10 w-auto object-contain rounded-sm"
                    />
                )}

                <button onClick={onToggle} className="text-muted-foreground hover:text-foreground transition-colors">
                    {isOpen ? <ChevronLeft size={20} /> : <ChevronRight size={20} />}
                </button>
            </div>

            {/* Navigation Buttons */}
            <div className="flex-1 py-4 flex flex-col gap-2 px-2">
                <SidebarButton 
                    icon={<Table size={18} />} 
                    label="Trades"
                    isOpen={isOpen}
                    onClick={() => onOpenPanel("blotter", "Trade Blotter")}
                />
                <SidebarButton 
                    icon={<ChartCandlestick size={18} />}
                    label="Markets"
                    isOpen={isOpen}
                    onClick={() => onOpenPanel("markets", "Markets")}
                />
                <SidebarButton 
                    icon={<Database size={18} />}
                    label="RefData"
                    isOpen={isOpen}
                    onClick={() => onOpenPanel("refdata", "RefData", "refDataGrid")}
                />
            </div>

            {/* Footer / Command Palette Trigger */}
            <div className={`border-t border-border flex justify-center ${isOpen ? 'p-4' : 'py-4 px-2'}`}>
                <Button 
                    variant="default" 
                    className={`shadow-md font-mono transition-all overflow-hidden ${
                        isOpen ? 'w-full justify-between px-3' : 'w-8 h-8 p-0 justify-center'
                    }`}
                    onClick={onOpenCmdk}
                    title="Search Commands (Ctrl+K)"
                >
                {isOpen ? (
                    <>
                    <span className="truncate text-xs">Search Commands</span>
                    <kbd className="hidden sm:inline-block bg-primary-foreground/20 text-primary-foreground rounded px-1.5 py-0.2 ml-2">Ctrl+K</kbd>
                    </>
                ) : (
                    <Search size={10} />
                )}
                </Button>
            </div>

        </div>
    )
}

