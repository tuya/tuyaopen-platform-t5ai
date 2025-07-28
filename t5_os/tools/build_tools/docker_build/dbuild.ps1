#!/usr/bin/env pwsh

$global:DOCKER_IMAGE_LOWEST_VERSION = "1.0"
$global:DOCKER_IMAGE_VERSION = ""
$global:DOCKER_IMAGE = "bekencorp/armino-idk"

function Check-DockerInstalled {
    $dockerExecutablePath = Get-Command docker -ErrorAction SilentlyContinue
    if (!($dockerExecutablePath)) {
        Write-Host "You don't have docker installed in your path.`nPlease find our quick start to build using docker client."
        exit 1
    }
}

function Check-DockerRunning {
    try {
        $dockerInfo = docker ps
        if (!($?)) {
            Write-Host "Docker not started, please read the user manual to start Docker."
            exit 1
        }
    } catch {
        Write-Host "docker running error"
        exit 1
    }
}

function Compare-Versions {
    param (
        [string]$version1,
        [string]$version2
    )

    $ver1Main, $ver1Sub = $version1 -split '\.'
    $ver2Main, $ver2Sub = $version2 -split '\.'

    $ver1Main = [int]$ver1Main
    $ver1Sub = [int]$ver1Sub
    $ver2Main = [int]$ver2Main
    $ver2Sub = [int]$ver2Sub

    if ($ver1Main -gt $ver2Main) {
        return 1
    } elseif ($ver1Main -lt $ver2Main) {
        return -1
    } else {
        if ($ver1Sub -gt $ver2Sub) {
            return 1
        } elseif ($ver1Sub -lt $ver2Sub) {
            return -1
        } else {
            return 0
        }
    }
}

function Get-MaxVersion {
    param (
        [string[]]$versions
    )

    if ($versions.Length -eq 0) {
        return $null
    }

    $maxVersion = $versions[0]

    for ($i = 1; $i -lt $versions.Length; $i++) {
        $currentVersion = $versions[$i]
        $comparisonResult = Compare-Versions -version1 $maxVersion -version2 $currentVersion

        if ($comparisonResult -lt 0) {
            $maxVersion = $currentVersion
        }
    }

    return $maxVersion
}


function Check-ImageExist {
    $images_info = docker images --format "{{.Repository}} {{.Tag}}" | Select-String $DOCKER_IMAGE | ForEach-Object { $_.Line.Split(" ")[1] }
    if (-not $images_info) {
        Write-Host "Docker build image does not exist, please read the user manual to install image."
        exit 1
    }

    $max_version = Get-MaxVersion -versions $images_info
    $result = Compare-Versions -version1 $max_version -version2 $DOCKER_IMAGE_LOWEST_VERSION
    if ($result -ge 0) {
        $global:DOCKER_IMAGE_VERSION = $max_version
    } else {
        Write-Host "Docker image version is outdated. The minimum version is $DOCKER_IMAGE_LOWEST_VERSION"
        exit 1
    }
}

function Run-DockerBuild {
    $args_array = $args -split " "
    docker run --rm -v ${PWD}:/armino -w /armino ${DOCKER_IMAGE}:${DOCKER_IMAGE_VERSION} $args_array
}

function main {
    Check-DockerInstalled
    Check-DockerRunning
    Check-ImageExist
    Run-DockerBuild "$args"
}

main "$args"
