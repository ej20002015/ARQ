import { AlertCircle } from "lucide-react";
import { Button } from "@/components/ui/button";

interface ErrorStateProps {
    title?: string;
    message: string;
    onRetry?: () => void;
    isRetrying?: boolean;
}

export function ErrorState({ 
    title = "An error occurred", 
    message, 
    onRetry, 
    isRetrying = false 
}: ErrorStateProps) {
    return (
        <div className="absolute inset-0 flex flex-col items-center justify-center bg-neutral-950/50 p-6 text-center z-50">
            <AlertCircle className="h-10 w-10 text-destructive mb-4 opacity-80" />
            <h3 className="text-lg font-semibold text-foreground mb-1">
                {title}
            </h3>
            <p className="text-sm text-muted-foreground max-w-md mb-6">
                {message}
            </p>
            {onRetry && (
                <Button 
                    variant="default" 
                    onClick={onRetry}
                    disabled={isRetrying}
                >
                    {isRetrying ? "Retrying..." : "Try Again"}
                </Button>
            )}
        </div>
    );
}