#!/usr/bin/env groovy
@Library('rocJenkins@pong') _
import com.amd.project.*
import com.amd.docker.*

def runCI =
{
    nodeDetails, jobName->
    
    def prj = new rocProject('rocPyDecode', 'PreCheckin')

    def nodes = new dockerNodes(nodeDetails, jobName, prj)

    def commonGroovy

    boolean formatCheck = false
     
    def compileCommand =
    {
        platform, project->

        commonGroovy = load "${project.paths.project_src_prefix}/.jenkins/common.groovy"
        commonGroovy.runCompileCommand(platform, project, jobName)
    }

    
    def testCommand =
    {
        platform, project->

        commonGroovy.runTestCommand(platform, project)
    }

    def packageCommand =
    {
        platform, project->

        commonGroovy.runPackageCommand(platform, project)
    }

    buildProject(prj, formatCheck, nodes.dockerArray, compileCommand, testCommand, packageCommand)
}

ci: { 
    String urlJobName = auxiliary.getTopJobName(env.BUILD_URL)

    def propertyList = []
    auxiliary.appendPropertyList(propertyList)

    def jobNameList = []
    auxiliary.appendJobNameList(jobNameList)

    propertyList.each 
    {
        jobName, property->
        if (urlJobName == jobName) {
            properties(auxiliary.addCommonProperties(property))
        }
    }

    jobNameList.each
    {
        jobName, nodeDetails->
        if (urlJobName == jobName) {
            stage(jobName) {
                runCI(nodeDetails, jobName)
            }
        }
    }
}
