export default function LoadingSpinner() {
    return (
        <div
            role="status"
            aria-label="Loading"
            className="flex flex-col items-center justify-center p-4">
            <div className="w-8 h-8 border-4 border-blue-500 border-t-transparent rounded-full animate-spin"></div>
            <span className="mt-2 text-blue-700">Searching...</span>
        </div>
    );

}