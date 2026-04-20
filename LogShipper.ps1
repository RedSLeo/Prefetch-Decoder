# -- Variables --

$csvPath = ".\evidence_log.csv"
$sqlServer = "DESKTOP-IABBKOD"
$database = "ForensicsDB"

# -- Verify if C++ tool actually generated the log file
if (-Not (Test-Path $csvPath)) {
    Write-Host "[-] No evidence log found. Exiting" -ForegroundColor Red
    exit
}

Write-Host "[+] Evidence Log Discovered. Reading Data..." -ForegroundColor Cyan

# Read the CSV. Defining the headers
$logs = Import-Csv $csvPath -Header "Executable", "Hash"

# Create SQL Connection (Utilizing: Windows Authentication)
$connectionString = "Server=$sqlServer; Database=$database; Integrated Security=True;"
$connection = New-Object System.Data.SQLClient.SqlConnection($connectionString)

try {
     $connection.Open()
     Write-Host "[+] Successfully connected to SQL Server." -ForegroundColor Green

     foreach ($log in $logs) {
        $exe = $log.ExecutableName
        $hash = $log.Hash
        $date = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")

        # Create SQL insert command
        $query = "INSERT INTO PrefetchLogs (RunDate, ExecutableName, Hash) VALUES ('$date', '$exe', '$hash')"

        $command = $connection.CreateCommand()
        $command.CommandText = $query

        # Execute the query
        $command.ExecuteNonQuery() | Out-Null

        Write-Host " -> Inserted: $exe ($hash)" -ForegroundColor Yellow
     }

} catch {
    Write-Host "[-] Database Error: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    $connection.Close()
    Write-Host "[+] Pipeline Complete. Connection Closed." -ForegroundColor Cyan
}